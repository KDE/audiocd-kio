/*
 * Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
 * Copyright (C) 2000-2002 Michael Matz <matz@kde.org>
 * Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
 * Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
 * Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>
 * Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2004-2005 Benjamin Meyer <ben at meyerhome dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifndef AUDIO_CD_H
#define AUDIO_CD_H

#include <KIO/WorkerBase>

class AudioCDEncoder;

struct cdrom_drive;

namespace AudioCD
{

/**
 * The core class of the audiocd:// KIO worker.
 * It has the KIO worker login and the ripping logic. The actual encoding
 * is done by encoders that are separate objects.
 */
class AudioCDProtocol : public KIO::WorkerBase
{
public:
    AudioCDProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    ~AudioCDProtocol() override;

    KIO::WorkerResult get(const QUrl &) override;
    KIO::WorkerResult stat(const QUrl &) override;
    KIO::WorkerResult listDir(const QUrl &) override;

protected:
    AudioCDEncoder *encoderFromExtension(const QString &extension);
    AudioCDEncoder *determineEncoder(const QString &filename);

    struct cdrom_drive *findDrive(bool &noPermission);
    void parseURLArgs(const QUrl &);

    void loadSettings();

    /**
     * From the request information (Private member "d"),
     * get the first and last sector for the request.
     * return false if the parameters are invalid (for instance,
     * track number which does not exist on this CD)
     */
    bool getSectorsForRequest(struct cdrom_drive *drive, long &firstSector, long &lastSector) const;

    /**
     * Give the size in bytes of the space between those two
     * sectors, depending on the file type.
     */
    long fileSize(long firstSector, long lastSector, AudioCDEncoder *encoder);

    /**
     * The heart of this KIO worker.
     * Reads data off the cd and then passes it to an encoder to encode
     */
    KIO::WorkerResult
    paranoiaRead(struct cdrom_drive *drive, long firstSector, long lastSector, AudioCDEncoder *encoder, const QString &fileName, unsigned long size);

    KIO::WorkerResult initRequest(const QUrl &url, struct cdrom_drive **drive);

    /**
     * Add an entry in the KIO directory, using the title you give,
     * it will set the extension itself depending on the fileType.
     * You must also give the trackNumber for the size of the file
     * to be calculated.
     * @note If you want to add a file which is the whole CD, give
     * trackNo = -1
     *
     * @note You can always safely add MP3 or OGG files. The function
     * will check if kio_audiocd was compiled with support for those,
     * so NO NEED to wrap your calls with #ifdef for lame or vorbis.
     * this function will do the right thing.
     */
    void addEntry(const QString &trackTitle, AudioCDEncoder *encoder, struct cdrom_drive *drive, int trackNo);

    class Private;
    Private *d;

private:
    void generateTemplateTitles();
    KIO::WorkerResult checkNoHost(const QUrl &url);

    QList<AudioCDEncoder *> encoders;
    KIO::WorkerResult getDrive(struct cdrom_drive **drive);

    // These are the only guaranteed encoders to be built, the rest
    // are dynamic depending on other libraries on the system
    AudioCDEncoder *encoderTypeCDA;
    AudioCDEncoder *encoderTypeWAV;
};

} // end namespace AudioCD

#endif // AUDIO_CD_H
