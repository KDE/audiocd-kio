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
#include <klibloader.h>
#include <kdebug.h>
#include <QDir>
#include <QRegExp>
#include <kstandarddirs.h>

/**
 * Attempt to load a plugin and see if it has the symbol create_audiocd_encoders.
 * @param libFileName file to try to load.
 * @returns pointer to the symbol or NULL
 */
KLibrary::void_function_ptr loadPlugin(const QString &libFileName)
{
#ifdef DEBUG
    kDebug(7117) << "Trying to load library. File: \"" << libFileName.latin1() << "\".";
#endif
    KLibLoader *loader = KLibLoader::self();
    if (!loader)
        return NULL;
#ifdef DEBUG
    kDebug(7117) << "We have a loader. File:  \"" << libFileName.latin1() << "\".";
#endif
    KLibrary *lib = loader->library(libFileName.toLatin1());
    if (!lib)
        return NULL;
#ifdef DEBUG
    kDebug(7117) << "We have a library. File: \"" << libFileName.latin1() << "\".";
#endif
    KLibrary::void_function_ptr cplugin = lib->resolveFunction("create_audiocd_encoders");
    if (!cplugin)
        return NULL;
#ifdef DEBUG
    kDebug(7117) << "We have a plugin. File:  \"" << libFileName.latin1() << "\".";
#endif
    return cplugin;
}

/**
 * There might be a "better" way of doing this, but I don't know it,
 * but I do know that this does work. :)  Feel free to improve the loading system,
 * there isn't much code anyway.
 */
void AudioCDEncoder::findAllPlugins(KIO::SlaveBase *slave, QList<AudioCDEncoder *>&encoders)
{
    QString foundEncoders;

		KStandardDirs standardDirs;
    QStringList dirs = standardDirs.findDirs("module", "");
    for (QStringList::const_iterator it = dirs.constBegin(); it != dirs.constEnd(); ++it) {
        QDir dir(*it);
        if (!dir.exists()) {
            kDebug(7117) << "Directory given by KStandardDirs: " << dir.path() << " doesn't exists!";
            continue;
        }

        dir.setFilter(QDir::Files | QDir::Hidden);
        const QFileInfoList files = dir.entryInfoList();
        for (int i = 0; i < files.count(); ++i) {
            QFileInfo fi(files.at(i));
            if (0 < fi.fileName().count(QRegExp("^libaudiocd_encoder_.*.so$"))) {
                QString fileName = (fi.fileName().mid(0, fi.fileName().indexOf('.')));
                if (foundEncoders.contains(fileName)) {
                    kDebug(7117) << "Warning, encoder has been found twice!";
                    continue;
                }
                foundEncoders.append(fileName);
                KLibrary::void_function_ptr function = loadPlugin(fileName);
                if (function) {
                    void (*functionPointer) (KIO::SlaveBase *, QList<AudioCDEncoder*>&) =
                            (void (*)(KIO::SlaveBase *slave, QList<AudioCDEncoder *>&encoders)) function;
                    functionPointer(slave, encoders);
                }
            }
        }
    }
}

