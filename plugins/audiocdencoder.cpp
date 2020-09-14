/*
  Copyright (C) 2004, 2005 Benjamin Meyer <ben at meyerhome dot net>

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

#include "audiocdencoder.h"
#include "audiocd_kio_debug.h"

#include <QDir>
#include <QLibrary>
#include <QLibraryInfo>
#include <QRegularExpression>
#include <QCoreApplication>

Q_LOGGING_CATEGORY(AUDIOCD_KIO_LOG, "kf.kio.slaves.audiocd")

/**
 * Attempt to load a plugin and see if it has the symbol create_audiocd_encoders.
 * @param libFileName file to try to load.
 * @returns pointer to the symbol or NULL
 */
static QFunctionPointer loadPlugin(const QString &libFileName)
{
    qCDebug(AUDIOCD_KIO_LOG) << "Trying to load" << libFileName;
    QFunctionPointer cplugin = QLibrary(libFileName).resolve("create_audiocd_encoders");
    if (!cplugin)
        return nullptr;
    qCDebug(AUDIOCD_KIO_LOG) << "Loaded plugin";
    return cplugin;
}

/**
 * There might be a "better" way of doing this, but I don't know it
 * and I do know that this does work. :)  Feel free to improve the loading system,
 * there isn't much code anyway.
 */
void AudioCDEncoder::findAllPlugins(KIO::SlaveBase *slave, QList<AudioCDEncoder *>&encoders)
{
    QString foundEncoders;

    for (const QString &path : QCoreApplication::libraryPaths()) {
        QDir dir(path);
        if (!dir.exists()) {
            //qCDebug(AUDIOCD_KIO_LOG) << "Library path" << path << "does not exist";
            continue;
        }

        dir.setFilter(QDir::Files);
        const QFileInfoList files = dir.entryInfoList();

        for (const QFileInfo &fi : qAsConst(files)) {
            if (fi.fileName().contains(QRegularExpression(QLatin1String("^libaudiocd_encoder_.*.so$")))) {
                QString fileName = fi.baseName();

                if (foundEncoders.contains(fileName)) {
                    qCWarning(AUDIOCD_KIO_LOG) << "Encoder" << fileName << "found in multiple locations";
                    continue;
                }
                foundEncoders.append(fileName);

                QFunctionPointer function = loadPlugin(fi.absoluteFilePath());
                if (function) {
                    void (*functionPointer) (KIO::SlaveBase *, QList<AudioCDEncoder*>&) =
                        (void (*)(KIO::SlaveBase *slave, QList<AudioCDEncoder *>&encoders)) function;
                    functionPointer(slave, encoders);
                }
            }
        }
    }
}
