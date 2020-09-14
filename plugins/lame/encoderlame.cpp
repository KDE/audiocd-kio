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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <config-audiocd.h>

#include "encoderlame.h"
#include "audiocd_lame_encoder.h"
#include "audiocd_kio_debug.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryFile>

Q_LOGGING_CATEGORY(AUDIOCD_KIO_LOG, "kf.kio.slaves.audiocd")

extern "C"
{
	AUDIOCDPLUGINS_EXPORT void create_audiocd_encoders(KIO::SlaveBase *slave, QList<AudioCDEncoder*> &encoders) {
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
	QTemporaryFile *tempFile;
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
	EncoderLameConfig *config = new EncoderLameConfig();
	config->cbr_settings->hide();
	return config;
}

bool EncoderLame::init(){
	// Determine if lame is installed on the system or not.
	if ( QStandardPaths::findExecutable( QStringLiteral( "lame" )).isEmpty() )
		return false;

	// Ask lame for the list of genres it knows; otherwise it barfs when doing
	// e.g. lame --tg 'Vocal Jazz'
	KProcess proc;
	proc.setOutputChannelMode(KProcess::MergedChannels);
	proc << QStringLiteral("lame") << QStringLiteral("--genre-list");
	proc.execute();

	if(proc.exitStatus() != QProcess::NormalExit)
		return false;

	QByteArray array = proc.readAll();
	QString str = QString::fromLocal8Bit( array );
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	d->genreList = str.split(QLatin1Char('\n'), QString::SkipEmptyParts );
#else
	d->genreList = str.split(QLatin1Char('\n'), Qt::SkipEmptyParts );
#endif
	// Remove the numbers in front of every genre
	for( QStringList::Iterator it = d->genreList.begin(); it != d->genreList.end(); ++it ) {
		QString& genre = *it;
		int i = 0;
		while ( i < genre.length() && ( genre[i].isSpace() || genre[i].isDigit() ) )
			++i;
		genre = genre.mid( i );

	}
	//qCDebug(AUDIOCD_KIO_LOG) << "Available genres:" << d->genreList;

	return true;
}

void EncoderLame::loadSettings(){
	// Generate the command line arguments for the current settings
	args.clear();

	Settings *settings = Settings::self();


	// Should we bitswap? I'm unsure about the proper logic for this.
	// KDE3 always swapped on little-endian hosts,
	// while #171065 says we shouldn't always do so.
	// So... let's make it configurable, at least.

	bool swap = false;
	switch (settings->byte_swap()) {
	case Settings::EnumByte_swap::Yes:
		swap = true;
		break;
	case Settings::EnumByte_swap::No:
		swap = false;
		break;
	default:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		swap = false; // it looks like lame is now clever enough to do the right thing by default? (#171065)
#else
		swap = false;
#endif
        }
	if (swap)
		args << QStringLiteral("-x");

	int quality = settings->quality();
	if (quality < 0 ) quality = quality *-1;
	if (quality > 9) quality = 9;

	int method = settings->bitrate_constant() ? 0 : 1 ;

	if (method == 0) {
		// Constant Bitrate Encoding
		args.append(QStringLiteral("-b"));
		args.append(QStringLiteral("%1").arg(bitrates[settings->cbr_bitrate()]));
		d->bitrate = bitrates[settings->cbr_bitrate()];
		args.append(QStringLiteral("-q"));
		args.append(QStringLiteral("%1").arg(quality));
	}
	else {
		// Variable Bitrate Encoding
		if (settings->vbr_average_br()) {
			args.append(QStringLiteral("--abr"));
			args.append(QStringLiteral("%1").arg(bitrates[settings->vbr_mean_brate()]));
			d->bitrate = bitrates[settings->vbr_mean_brate()];
			if (settings->vbr_min_br()){
				args.append(QStringLiteral("-b"));
				args.append(QStringLiteral("%1").arg(bitrates[settings->vbr_min_brate()]));
			}
			if (settings->vbr_min_hard())
				args.append(QStringLiteral("-F"));
			if (settings->vbr_max_br()){
				args.append(QStringLiteral("-B"));
				args.append(QStringLiteral("%1").arg(bitrates[settings->vbr_max_brate()]));
			}
		} else {
			d->bitrate = 128;
			args.append(QStringLiteral("-V"));
			args.append(QStringLiteral("%1").arg(quality));
		}
		if ( !settings->vbr_xing_tag() )
			args.append(QStringLiteral("-t"));
	}

	args.append(QStringLiteral("-m"));
	switch ( settings->stereo() ) {
		case 0:
			args.append(QStringLiteral("s"));
			break;
		case 1:
			args.append(QStringLiteral("j"));
			break;
		case 2:
			args.append(QStringLiteral("d"));
			break;
		case 3:
			args.append(QStringLiteral("m"));
			break;
		default:
			args.append(QStringLiteral("s"));
			break;
	}

	if(settings->copyright())
		args.append(QStringLiteral("-c"));
	if(!settings->original())
		args.append(QStringLiteral("-o"));
	if(settings->iso())
		args.append(QStringLiteral("--strictly-enforce-ISO"));
	if(settings->crc())
		args.append(QStringLiteral("-p"));

	if ( settings->enable_lowpass() ) {
		args.append(QStringLiteral("--lowpass"));
		args.append(QStringLiteral("%1").arg(settings->lowfilterfreq()));

		if (settings->set_lpf_width()){
			args.append(QStringLiteral("--lowpass-width"));
			args.append(QStringLiteral("%1").arg(settings->lowfilterwidth()));
		}
	}

	if ( settings->enable_highpass()) {
		args.append(QStringLiteral("--hipass"));
		args.append(QStringLiteral("%1").arg(settings->highfilterfreq()));

		if (settings->set_hpf_width()){
			args.append(QStringLiteral("--hipass-width"));
			args.append(QStringLiteral("%1").arg(settings->highfilterwidth()));
		}
	}
}

unsigned long EncoderLame::size(long time_secs) const {
	return (time_secs * d->bitrate * 1000)/8;
}

long EncoderLame::readInit(long /*size*/){
	// Create KProcess
	d->currentEncodeProcess	= new KProcess();
	d->tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/kaudiocd_XXXXXX") + QLatin1String(".mp3"));
	d->tempFile->open();
	d->lastErrorMessage.clear();
	d->processHasExited = false;

	// -r raw/pcm
	// -s 44.1 (because it is raw you have to specify this)
	*(d->currentEncodeProcess) << QStringLiteral("lame") << QStringLiteral("--verbose") << QStringLiteral("-r") << QStringLiteral("-s") << QStringLiteral("44.1");
	*(d->currentEncodeProcess) << args;
	if(Settings::self()->id3_tag())
		*d->currentEncodeProcess << trackInfo;

	// Read in stdin, output to the temp file
	*d->currentEncodeProcess << QStringLiteral("-") << d->tempFile->fileName();

	//qCDebug(AUDIOCD_KIO_LOG) << d->currentEncodeProcess->args();


	connect(d->currentEncodeProcess, &KProcess::readyReadStandardOutput,
                         this, &EncoderLame::receivedStdout);
	connect(d->currentEncodeProcess, &KProcess::readyReadStandardError,
                         this, &EncoderLame::receivedStderr);
// 	connect(d->currentEncodeProcess, &KProcess::bytesWritten,
//                          this, &EncoderLame::wroteStdin);

	connect(d->currentEncodeProcess, QOverload<int, QProcess::ExitStatus>::of(&KProcess::finished),
                         this, &EncoderLame::processExited);

	// Launch!
	d->currentEncodeProcess->setOutputChannelMode(KProcess::SeparateChannels);
	d->currentEncodeProcess->start();
	return 0;
}

void EncoderLame::processExited ( int exitCode, QProcess::ExitStatus /*status*/ ){
	qCDebug(AUDIOCD_KIO_LOG) << "Lame Encoding process exited with: " << exitCode;
	d->processHasExited = true;
}

void EncoderLame::receivedStderr(){
	QByteArray error = d->currentEncodeProcess->readAllStandardError();
	qCDebug(AUDIOCD_KIO_LOG) << "Lame stderr: " << error;
	if ( !d->lastErrorMessage.isEmpty() )
		d->lastErrorMessage += QLatin1Char('\t');
	d->lastErrorMessage += QString::fromLocal8Bit( error );
}

void EncoderLame::receivedStdout(){
	QString output = QString::fromLocal8Bit(d->currentEncodeProcess->readAllStandardOutput());
	qCDebug(AUDIOCD_KIO_LOG) << "Lame stdout: " << output;
}

// void EncoderLame::wroteStdin(){
// 	d->waitingForWrite = false;
// }

long EncoderLame::read(qint16 *buf, int frames){
	if(!d->currentEncodeProcess)
		return 0;
        if (d->processHasExited)
		return -1;

	// Pipe the raw data to lame
	char * cbuf = reinterpret_cast<char *>(buf);
	d->currentEncodeProcess->write(cbuf, frames * 4);
	// We can't return until the buffer has been written
	d->currentEncodeProcess->waitForBytesWritten(-1);

	// Determine the file size increase
	QFileInfo file(d->tempFile->fileName());
	uint change = file.size() - d->lastSize;
	d->lastSize = file.size();
	return change;
}

long EncoderLame::readCleanup(){
	if(!d->currentEncodeProcess)
		return 0;

	// Let lame tag the first frame of the mp3
	d->currentEncodeProcess->closeWriteChannel();
	d->currentEncodeProcess->waitForFinished(-1);

	// Now copy the file out of the temp into kio
	QFile file( d->tempFile->fileName() );
	if ( file.open( QIODevice::ReadOnly ) ) {
		char data[1024];
		while ( !file.atEnd() ) {
			uint read = file.read(data, 1024);
			QByteArray output(data, read);
			ioslave->data(output);
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
	trackInfo.append(QStringLiteral("--tt"));
	trackInfo.append(info.track(track-1).get(Title).toString());

	trackInfo.append(QStringLiteral("--ta"));
	trackInfo.append(info.track(track-1).get(Artist).toString());

	trackInfo.append(QStringLiteral("--tl"));
	trackInfo.append(info.get(Title).toString());

	trackInfo.append(QStringLiteral("--ty"));
	trackInfo.append(QStringLiteral("%1").arg(info.get(Year).toString()));

	trackInfo.append(QStringLiteral("--tc"));
	trackInfo.append(comment);

	trackInfo.append(QStringLiteral("--tn"));
	trackInfo.append(QStringLiteral("%1").arg(track));

	const QString genre = info.get(Genre).toString();
	if ( d->genreList.indexOf( genre ) != -1 )
	{
	trackInfo.append(QStringLiteral("--tg"));
		trackInfo.append(genre);
	}
}


QString EncoderLame::lastErrorMessage() const
{
	return d->lastErrorMessage;
}
