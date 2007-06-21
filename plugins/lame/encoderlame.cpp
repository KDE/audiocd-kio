/*
  Copyright (C) 2005 Benjamin Meyer <ben at meyerhome dot net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <config.h>
#include <endian.h>
#include "encoderlame.h"
#include "encoderlameconfig.h"
#include "audiocd_lame_encoder.h"

#include <kdebug.h>
#include <qgroupbox.h>
#include <kprocess.h>
#include <kdebug.h>

#include <kglobal.h>
#include <klocale.h>
#include <kapplication.h>
#include <qfileinfo.h>
#include <ktempfile.h>
#include <kstandarddirs.h>
#include "collectingprocess.h"

extern "C"
{
	KDE_EXPORT void create_audiocd_encoders(KIO::SlaveBase *slave, QPtrList<AudioCDEncoder> &encoders) {
		encoders.append(new EncoderLame(slave));
	}
}

static int bitrates[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };

class EncoderLame::Private
{
public:
	int bitrate;
	bool waitingForWrite;
	bool processHasExited;
	QString lastErrorMessage;
	QStringList genreList;
	uint lastSize;
	KProcess *currentEncodeProcess;
	KTempFile *tempFile;
};

EncoderLame::EncoderLame(KIO::SlaveBase *slave) : QObject(), AudioCDEncoder(slave) {
	d = new Private();
	d->waitingForWrite = false;
	d->processHasExited = false;
	d->lastSize = 0;
	loadSettings();
}

EncoderLame::~EncoderLame(){
	delete d;
}

QWidget* EncoderLame::getConfigureWidget(KConfigSkeleton** manager) const {
	(*manager) = Settings::self();
	KGlobal::locale()->insertCatalogue("audiocd_encoder_lame");
	EncoderLameConfig *config = new EncoderLameConfig();
	config->cbr_settings->hide();
	return config;
}

bool EncoderLame::init(){
	// Determine if lame is installed on the system or not.
	if ( KStandardDirs::findExe( "lame" ).isEmpty() )
		return false;

	// Ask lame for the list of genres it knows; otherwise it barfs when doing
	// e.g. lame --tg 'Vocal Jazz'
    CollectingProcess proc;
	proc << "lame" << "--genre-list";
	proc.start(KProcess::Block, KProcess::Stdout);

	if(proc.exitStatus() != 0)
		return false;

	const QByteArray data = proc.collectedStdout();
	QString str;
	if ( !data.isEmpty() )
		str = QString::fromLocal8Bit( data, data.size() );

	d->genreList = QStringList::split( '\n', str );
	// Remove the numbers in front of every genre
	for( QStringList::Iterator it = d->genreList.begin(); it != d->genreList.end(); ++it ) {
		QString& genre = *it;
		uint i = 0;
		while ( i < genre.length() && ( genre[i].isSpace() || genre[i].isDigit() ) )
			++i;
		genre = genre.mid( i );

	}
	//kdDebug(7117) << "Available genres:" << d->genreList << endl;

	return true;
}

void EncoderLame::loadSettings(){
	// Generate the command line arguments for the current settings
	args.clear();

	Settings *settings = Settings::self();

	int quality = settings->quality();
	if (quality < 0 ) quality = quality *-1;
	if (quality > 9) quality = 9;

	int method = settings->bitrate_constant() ? 0 : 1 ;

	if (method == 0) {
		// Constant Bitrate Encoding
		args.append("-b");
		args.append(QString("%1").arg(bitrates[settings->cbr_bitrate()]));
		d->bitrate = bitrates[settings->cbr_bitrate()];
		args.append("-q");
		args.append(QString("%1").arg(quality));
	}
	else {
		// Variable Bitrate Encoding
		if (settings->vbr_average_br()) {
			args.append("--abr");
			args.append(QString("%1").arg(bitrates[settings->vbr_mean_brate()]));
			d->bitrate = bitrates[settings->vbr_mean_brate()];
			if (settings->vbr_min_br()){
				args.append("-b");
				args.append(QString("%1").arg(bitrates[settings->vbr_min_brate()]));
			}
			if (settings->vbr_min_hard())
				args.append("-F");
			if (settings->vbr_max_br()){
				args.append("-B");
				args.append(QString("%1").arg(bitrates[settings->vbr_max_brate()]));
			}
		} else {
			d->bitrate = 128;
			args.append("-V");
			args.append(QString("%1").arg(quality));
		}
		if ( !settings->vbr_xing_tag() )
			args.append("-t");
	}

	args.append("-m");
	switch ( settings->stereo() ) {
		case 0:
			args.append("s");
			break;
		case 1:
			args.append("j");
			break;
		case 2:
			args.append("d");
			break;
		case 3:
			args.append("m");
			break;
		default:
			args.append("s");
			break;
	}

	if(settings->copyright())
		args.append("-c");
	if(!settings->original())
		args.append("-o");
	if(settings->iso())
		args.append("--strictly-enforce-ISO");
	if(settings->crc())
		args.append("-p");

	if ( settings->enable_lowpass() ) {
		args.append("--lowpass");
		args.append(QString("%1").arg(settings->lowfilterfreq()));

		if (settings->set_lpf_width()){
			args.append("--lowpass-width");
			args.append(QString("%1").arg(settings->lowfilterwidth()));
		}
	}

	if ( settings->enable_highpass()) {
		args.append("--hipass");
		args.append(QString("%1").arg(settings->highfilterfreq()));

		if (settings->set_hpf_width()){
			args.append("--hipass-width");
			args.append(QString("%1").arg(settings->highfilterwidth()));
		}
	}
}

unsigned long EncoderLame::size(long time_secs) const {
	return (time_secs * d->bitrate * 1000)/8;
}

long EncoderLame::readInit(long /*size*/){
	// Create KProcess
	d->currentEncodeProcess	= new KProcess(0);
	QString prefix = locateLocal("tmp", "");
	d->tempFile = new KTempFile(prefix, ".mp3");
	d->tempFile->setAutoDelete(true);
	d->lastErrorMessage = QString::null;
	d->processHasExited = false;

	// -x bitswap
	// -r raw/pcm
	// -s 44.1 (because it is raw you have to specify this)
#if __BYTE_ORDER == __LITTLE_ENDIAN
	*(d->currentEncodeProcess)	<< "lame" << "--verbose" << "-x" << "-r" << "-s" << "44.1";
#else
	*(d->currentEncodeProcess)      << "lame" << "--verbose" << "-r" << "-s" << "44.1";
#endif

	*(d->currentEncodeProcess) << args;
	if(Settings::self()->id3_tag())
		*d->currentEncodeProcess << trackInfo;

	// Read in stdin, output to the temp file
	*d->currentEncodeProcess << "-" << d->tempFile->name().latin1();

	//kdDebug(7117) << d->currentEncodeProcess->args() << endl;


	connect(d->currentEncodeProcess, SIGNAL(receivedStdout(KProcess *, char *, int)),
	                 this, SLOT(receivedStdout(KProcess *, char *, int)));
	connect(d->currentEncodeProcess, SIGNAL(receivedStderr(KProcess *, char *, int)),
	                 this, SLOT(receivedStderr(KProcess *, char *, int)));
	connect(d->currentEncodeProcess, SIGNAL(wroteStdin(KProcess *)),
	                 this, SLOT(wroteStdin(KProcess *)));

	connect(d->currentEncodeProcess, SIGNAL(processExited(KProcess *)),
	                 this, SLOT(processExited(KProcess *)));

	// Launch!
	d->currentEncodeProcess->start(KProcess::NotifyOnExit, KShellProcess::All);
	return 0;
}

void EncoderLame::processExited ( KProcess *process ){
	kdDebug(7117) << "Lame Encoding process exited with: " << process->exitStatus() << endl;
	d->processHasExited = true;
}

void EncoderLame::receivedStderr( KProcess * /*process*/, char *buffer, int /*buflen*/ ){
	kdDebug(7117) << "Lame stderr: " << buffer << endl;
	if ( !d->lastErrorMessage.isEmpty() )
		d->lastErrorMessage += '\t';
	d->lastErrorMessage += QString::fromLocal8Bit( buffer );
}

void EncoderLame::receivedStdout( KProcess * /*process*/, char *buffer, int /*length*/ ){
	kdDebug(7117) << "Lame stdout: " << buffer << endl;
}

void EncoderLame::wroteStdin( KProcess * /*procces*/ ){
	d->waitingForWrite = false;
}

long EncoderLame::read(int16_t *buf, int frames){
	if(!d->currentEncodeProcess)
		return 0;
        if (d->processHasExited)
		return -1;

	// Pipe the raw data to lame
	char * cbuf = reinterpret_cast<char *>(buf);
	d->currentEncodeProcess->writeStdin( cbuf, frames*4);

	// We can't return until the buffer has been written
	d->waitingForWrite = true;
	while(d->waitingForWrite && d->currentEncodeProcess->isRunning()){
		kapp->processEvents();
		usleep(1);
	}

	// Determine the file size increase
	QFileInfo file(d->tempFile->name());
	uint change = file.size() - d->lastSize;
	d->lastSize = file.size();
	return change;
}

long EncoderLame::readCleanup(){
	if(!d->currentEncodeProcess)
		return 0;

	// Let lame tag the first frame of the mp3
	d->currentEncodeProcess->closeStdin();
	while( d->currentEncodeProcess->isRunning()){
		kapp->processEvents();
		usleep(1);
	}

	// Now copy the file out of the temp into kio
	QFile file( d->tempFile->name() );
	if ( file.open( IO_ReadOnly ) ) {
		QByteArray output;
		char data[1024];
		while ( !file.atEnd() ) {
			uint read = file.readBlock(data, 1024);
			output.setRawData(data, read);
			ioslave->data(output);
			output.resetRawData(data, read);
		}
		file.close();
	}

	// cleanup the process and temp
	delete d->currentEncodeProcess;
	delete d->tempFile;
	d->lastSize = 0;

	return 0;
}

void EncoderLame::fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment ){
	trackInfo.clear();
	trackInfo.append("--tt");
	trackInfo.append(info.trackInfoList[track].get("title").toString());

	trackInfo.append("--ta");
	trackInfo.append(info.get("artist").toString());

	trackInfo.append("--tl");
	trackInfo.append(info.get("title").toString());

	trackInfo.append("--ty");
	trackInfo.append(QString("%1").arg(info.get("year").toString()));

	trackInfo.append("--tc");
	trackInfo.append(comment);

	trackInfo.append("--tn");
	trackInfo.append(QString("%1").arg(track+1));

	const QString genre = info.get( "genre" ).toString();
	if ( d->genreList.find( genre ) != d->genreList.end() )
	{
		trackInfo.append("--tg");
		trackInfo.append(genre);
	}
}


QString EncoderLame::lastErrorMessage() const
{
	return d->lastErrorMessage;
}

#include "encoderlame.moc"
