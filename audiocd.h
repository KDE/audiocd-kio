/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
  Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>

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

#ifndef AUDIO_CD_H
#define AUDIO_CD_H

#include <qstring.h>

#include <kio/global.h>
#include <kio/slavebase.h>
#include <klibloader.h>

struct cdrom_drive;

namespace AudioCD {

class AudioCDProtocol : public KIO::SlaveBase
{
  public:

    AudioCDProtocol(const QCString & pool, const QCString & app);
    virtual ~AudioCDProtocol();

    virtual void get(const KURL &);
    virtual void stat(const KURL &);
    virtual void listDir(const KURL &);

  protected:

    enum FileType
    {
      FileTypeUnknown,
      FileTypeOggVorbis,
      FileTypeMP3,
      FileTypeWAV,
      FileTypeCDA
    };

    enum DirType
    {
      DirTypeUnknown,
      DirTypeDevice,
      DirTypeByName,
      DirTypeByTrack,
      DirTypeTitle,
      DirTypeInfo,
      DirTypeRoot,
      DirTypeMP3,
      DirTypeVorbis
    };

    static QString extension(enum FileType fileType);
    static FileType fileTypeFromExtension(const QString& extension);
    static FileType determineFiletype(const QString & filename);

    void                  writeHeader(long);
    struct cdrom_drive *  findDrive(bool &noPermission);
    void                  parseArgs(const KURL &);

    void getParameters();

    /**
     * From the request information (Private member "d"),
     * get the first and last sector for the request.
     * return false if the parameters are invalid (for instance,
     * track number which does not exist on this CD)
     */
    bool getSectorsForRequest(struct cdrom_drive * drive, long & firstSector, long & lastSector) const;

    /**
     * Give the size in bytes of this track, depending on
     * its file type.
     */
    long fileSize(struct cdrom_drive* drive, int trackNumber, FileType fileType);

    /**
     * Give the size in bytes of the space between those two
     * sectors, depending on the file type.
     */
    long fileSize(long firstSector, long lastSector, FileType fileType);

    void paranoiaRead(
        struct cdrom_drive  * drive,
        long                  firstSector,
        long                  lastSector,
        FileType              fileType
    );

    struct cdrom_drive *  initRequest(const KURL &);
    uint                  discid(struct cdrom_drive *);
    void                  updateCD(struct cdrom_drive *);
    
    /**
     * Add an entry in the KIO directory, using the title you give,
     * it will set the extension itself depending on the fileType.
     * You must also give the trackNumber for the size of the file
     * to be calculated.
     * NOTE: if you want to add a file which is the whole CD, give
     * trackNo = -1
     * NOTE2: you can safely add MP3 or OGG files always. the function
     * will check if kio_audiocd was compiled with support for those,
     * so NO NEED to wrap your calls with #ifdef for lame or vorbis.
     * this function will do the right thing.
     */
    void addEntry(const QString& trackTitle, enum FileType fileType, struct cdrom_drive * drive, int trackNo);
    FileType fileType(const QString & filename);

    class Private;
    Private * d;

  private:
    cdrom_drive * pickDrive();
    unsigned int get_discid(cdrom_drive *);
#ifdef HAVE_LAME
    KLibrary *_lamelib;
    bool      initLameLib();
#endif
#ifdef HAVE_VORBIS
    long vorbisSize(long time_secs);
#endif
};

}; // end namespace AudioCD

#endif
// vim:ts=2:sw=2:tw=78:et:
