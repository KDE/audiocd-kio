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

#ifndef ENCODER_LAME_H
#define ENCODER_LAME_H

#include "audiocdencoder.h"

class KProcess;

/**
 * MP3 encoder using the LAME encoder.
 * Go to http://lame.sourceforge.net/ for lots of information.
 */
class EncoderLame : public QObject, public AudioCDEncoder {

Q_OBJECT

public:
	EncoderLame(KIO::SlaveBase *slave);
	~EncoderLame();

	virtual QString type() const { return "MP3"; };
	virtual bool init();
	virtual void loadSettings();
	virtual unsigned long size(long time_secs) const;
	virtual const char * fileType() const { return "mp3"; };
	virtual const char * mimeType() const { return "audio/x-mp3"; };
	virtual void fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment );
	virtual long readInit(long size);
	virtual long read(int16_t * buf, int frames);
	virtual long readCleanup();
	virtual QString lastErrorMessage() const;

	virtual QWidget* getConfigureWidget(KConfigSkeleton** manager) const;

protected slots:
	void wroteStdin(KProcess *proc);
	void receivedStdout(KProcess *, char *buffer, int length);
	void receivedStderr(KProcess *proc, char *buffer, int buflen);
	void processExited(KProcess *proc);

private:
	class Private;
	Private * d;

	QStringList args;
	QStringList trackInfo;
};

#endif // ENCODER_LAME_H

