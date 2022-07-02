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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
  USA.
*/

#ifndef ENCODER_LAME_H
#define ENCODER_LAME_H

#include "audiocdencoder.h"
#include "ui_encoderlameconfig.h"

#include <KProcess>

class EncoderLameConfig : public QWidget, public Ui::EncoderLameConfig
{
public:
    EncoderLameConfig(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

/**
 * MP3 encoder using the LAME encoder.
 * Go to https://lame.sourceforge.io/ for more information.
 */
class EncoderLame : public QObject, public AudioCDEncoder {
    Q_OBJECT

public:
    explicit EncoderLame(KIO::SlaveBase *slave);
    ~EncoderLame() override;

    QString type() const override
    {
        return QStringLiteral("MP3");
    }
    bool init() override;
    void loadSettings() override;
    unsigned long size(long time_secs) const override;
    const char *fileType() const override
    {
        return "mp3";
    }
    const char *mimeType() const override
    {
        return "audio/x-mp3";
    }
    void fillSongInfo(KCDDB::CDInfo info, int track, const QString &comment) override;
    long readInit(long size) override;
    long read(qint16 *buf, int frames) override;
    long readCleanup() override;
    QString lastErrorMessage() const override;

    QWidget *getConfigureWidget(KConfigSkeleton **manager) const override;

protected Q_SLOTS:
    // 	void wroteStdin();
    void receivedStdout();
    void receivedStderr();
    void processExited(int exitCode, QProcess::ExitStatus /*status*/);

private:
    class Private;
    Private *d;

    QStringList args;
    QStringList trackInfo;
};

#endif // ENCODER_LAME_H
