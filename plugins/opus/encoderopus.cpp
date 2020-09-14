/*
  Copyright (C) 2005 Benjamin Meyer <ben at meyerhome dot net>
  Copyright (C) 2018 Yuri Chornoivan <yurchor@mageia.org>

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

#include "encoderopus.h"
#include "audiocd_opus_encoder.h"
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
		encoders.append(new EncoderOpus(slave));
	}
}

static const int bitrates[] = { 6, 12, 24, 48, 64, 80, 96, 128, 160, 192, 256 };

class EncoderOpus::Private
{
public:
	int bitrate;
	bool write_opus_comments;
	bool waitingForWrite;
	bool processHasExited;
	QString lastErrorMessage;
	uint lastSize;
	KProcess *currentEncodeProcess = nullptr;
	QTemporaryFile *tempFile = nullptr;
};

EncoderOpus::EncoderOpus(KIO::SlaveBase *slave) : QObject(), AudioCDEncoder(slave) {
	d = new Private();
	d->waitingForWrite = false;
	d->processHasExited = false;
	d->lastSize = 0;
	loadSettings();
}

EncoderOpus::~EncoderOpus(){
	delete d;
}

QWidget* EncoderOpus::getConfigureWidget(KConfigSkeleton** manager) const {
	(*manager) = Settings::self();
	EncoderOpusConfig *config = new EncoderOpusConfig();
	config->kcfg_opus_complexity->setRange(0, 10);
	config->kcfg_opus_complexity->setSingleStep(1);
	config->opus_bitrate_settings->hide();
	return config;
}

bool EncoderOpus::init(){
	// Determine if opusenc is installed on the system or not.
	if ( QStandardPaths::findExecutable( QStringLiteral( "opusenc" )).isEmpty() )
		return false;

	return true;
}

void EncoderOpus::loadSettings(){
	// Generate the command line arguments for the current settings
	args.clear();

	Settings *settings = Settings::self();

	if (settings->opus_enc_complexity()) {
		args.append(QStringLiteral("--comp"));
		args.append(QStringLiteral("%1").arg(settings->opus_complexity()));
	}
	else {
		// Constant Bitrate Encoding
		if (settings->set_opus_cbr()) {
			args.append(QStringLiteral("--bitrate"));
			args.append(QStringLiteral("%1").arg(bitrates[settings->opus_cbr()]));
			d->bitrate = settings->opus_cbr();
			args.append(QStringLiteral("--hard-cbr"));
		};
		// Constrained Variable Bitrate Encoding
		if (settings->set_opus_cvbr()) {
			args.append(QStringLiteral("--bitrate"));
			args.append(QStringLiteral("%1").arg(bitrates[settings->opus_cvbr()]));
			d->bitrate = bitrates[settings->opus_cvbr()];
			args.append(QStringLiteral("--cvbr"));
		};
		// Average Variable Bitrate Encoding
		if (settings->set_opus_vbr()) {
			args.append(QStringLiteral("--bitrate"));
			args.append(QStringLiteral("%1").arg(bitrates[settings->opus_vbr()]));
			d->bitrate = bitrates[settings->opus_vbr()];
			args.append(QStringLiteral("--vbr"));
		};
	};

	d->write_opus_comments = settings->opus_comments();

}

unsigned long EncoderOpus::size(long time_secs) const {
	return (time_secs * d->bitrate * 1000)/8;
}

long EncoderOpus::readInit(long /*size*/){
	// Create KProcess
	d->currentEncodeProcess	= new KProcess();
	d->tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/kaudiocd_XXXXXX") + QLatin1String(".opus"));
	d->tempFile->open();
	d->lastErrorMessage.clear();
	d->processHasExited = false;

	// --raw raw/pcm
	// --raw-rate 44100 (because it is raw you have to specify this)
	*(d->currentEncodeProcess) << QStringLiteral("opusenc") << QStringLiteral("--raw") << QStringLiteral("--raw-rate") << QStringLiteral("44100");
	*(d->currentEncodeProcess) << args;
	*d->currentEncodeProcess << trackInfo;

	// Read in stdin, output to the temp file
	*d->currentEncodeProcess << QStringLiteral("-") << d->tempFile->fileName();

	//qCDebug(AUDIOCD_KIO_LOG) << args;

	connect(d->currentEncodeProcess, &KProcess::readyReadStandardOutput,
                         this, &EncoderOpus::receivedStdout);
	connect(d->currentEncodeProcess, &KProcess::readyReadStandardError,
                         this, &EncoderOpus::receivedStderr);
	connect(d->currentEncodeProcess, QOverload<int, QProcess::ExitStatus>::of(&KProcess::finished),
                         this, &EncoderOpus::processExited);

	// Launch!
	d->currentEncodeProcess->setOutputChannelMode(KProcess::SeparateChannels);
	d->currentEncodeProcess->start();
	return 0;
}

void EncoderOpus::processExited ( int exitCode, QProcess::ExitStatus /*status*/ ){
	qCDebug(AUDIOCD_KIO_LOG) << "Opusenc Encoding process exited with: " << exitCode;
	d->processHasExited = true;
}

void EncoderOpus::receivedStderr(){
	QByteArray error = d->currentEncodeProcess->readAllStandardError();
	qCDebug(AUDIOCD_KIO_LOG) << "Opusenc stderr: " << error;
	if ( !d->lastErrorMessage.isEmpty() )
		d->lastErrorMessage += QLatin1Char('\t');
	d->lastErrorMessage += QString::fromLocal8Bit( error );
}

void EncoderOpus::receivedStdout(){
	QString output = QString::fromLocal8Bit(d->currentEncodeProcess->readAllStandardOutput());
	qCDebug(AUDIOCD_KIO_LOG) << "Opusenc stdout: " << output;
}

long EncoderOpus::read(qint16 *buf, int frames){
	if(!d->currentEncodeProcess)
		return 0;
        if (d->processHasExited)
		return -1;

	// Pipe the raw data to opusenc
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

long EncoderOpus::readCleanup(){
	if(!d->currentEncodeProcess)
		return 0;

	// Let opusenc tag the first frame of the opus
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

void EncoderOpus::fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment ){

	trackInfo.clear();

	if( !d->write_opus_comments )
		return;

	trackInfo.append(QStringLiteral("--album"));
	trackInfo.append(info.get(Title).toString());

	trackInfo.append(QStringLiteral("--artist"));
	trackInfo.append(info.track(track-1).get(Artist).toString());

	trackInfo.append(QStringLiteral("--title"));
	trackInfo.append(info.track(track-1).get(Title).toString());

	trackInfo.append(QStringLiteral("--date"));
	trackInfo.append( QDate(info.get(Year).toInt(), 1, 1 ).toString(Qt::ISODate) );

	trackInfo.append(QStringLiteral("--comment"));
	trackInfo.append(QStringLiteral("DESCRIPTION=") + comment);

	trackInfo.append(QStringLiteral("--comment"));
	trackInfo.append(QStringLiteral("TRACKNUMBER=") + QString::number(track));

	trackInfo.append(QStringLiteral("--genre"));
	trackInfo.append(QStringLiteral("%1").arg(info.get(Genre).toString()));

}

QString EncoderOpus::lastErrorMessage() const
{
	return d->lastErrorMessage;
}
