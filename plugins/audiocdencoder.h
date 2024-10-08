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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
  USA.
*/

#ifndef AUDIOCD_ENCODER_H
#define AUDIOCD_ENCODER_H

#include "audiocdplugins_export.h"

#include <KIO/WorkerBase>
#include <QList>
#include <QLoggingCategory>
#include <sys/types.h>

#include <KCDDB/CDInfo>

Q_DECLARE_LOGGING_CATEGORY(AUDIOCD_KIO_LOG)

class KConfigSkeleton;
using namespace KCDDB;

class AUDIOCDPLUGINS_EXPORT AudioCDEncoder
{
public:
    /**
     * Constructor.
     * @param worker parent that this classes can use to call data() with
     * when finished encoding bits.
     */
    explicit AudioCDEncoder(KIO::WorkerBase *worker)
        : ioWorker(worker)
    {
    }

    /**
     * Destructor.
     */
    virtual ~AudioCDEncoder()
    {
    }

    /**
     * Initializes the decoder, loading libraries, etc.  Encoders
     * that don't return true will will deleted and not used.
     * @returns false if unable to initialize the encoder.
     */
    virtual bool init() = 0;

    /**
     * The encoder should read in its config data here.
     */
    virtual void loadSettings() = 0;

    /**
     * Helper function to determine the end size of a
     * encoded file.
     * @param time_secs the length of the audio track in seconds.
     * @returns the size of a file if it is time_secs in length.
     */
    virtual unsigned long size(long time_secs) const = 0;

    /**
     * @returns the generic user string type/name of this encoder
     * Examples: "MP3", "Ogg Vorbis", "Wav", "FID Level 2", etc
     */
    virtual QString type() const = 0;

    /**
     * @returns the mime type for the files this encoder produces.
     * Example: "audio/x-wav"
     */
    virtual const char *mimeType() const = 0;

    /**
     * @returns the file type for the files this encoder produces.
     * Used in naming of the file for example foo.mp3
     * Examples: "mp3", "ogg", "wav"
     */
    virtual const char *fileType() const = 0;

    /**
     * Before the read functions are called this is
     * called to allow the encoders to store the cddb
     * information if they want to so it can be inserted
     * where necessary (start, middle, end, or combos etc).
     */
    virtual void fillSongInfo(KCDDB::CDInfo info, int track, const QString &comment) = 0;

    /**
     * Perform any initial file creation necessary for a new song (that
     * has just been sent via fillSongInfo())
     * @param size - the total binary size of the end file (via size()).
     * @return size of the data that was created by this function.
     */
    virtual long readInit(long size) = 0;

    /**
     * Passes a little bit of cd data to be encoded
     * This function is most likely called many many times.
     * @param buf pointer to the audio that has been read in so far
     * @param frames the number of frames of audio that are in @p buf
     * @return size of the data that was created by this function, -1 on error.
     */
    virtual long read(qint16 *buf, int frames) = 0;

    /**
     * Perform any final file creation/padding that is necessary
     * @return size of the data that was created by this function.
     */
    virtual long readCleanup() = 0;

    /**
     * Returns a configure widget for the encoder
     */
    virtual QWidget *getConfigureWidget(KConfigSkeleton **manager) const
    {
        Q_UNUSED(manager);
        return nullptr;
    }

    /**
     * Returns the last error message; called when e.g. read() returns -1.
     */
    virtual QString lastErrorMessage() const
    {
        return QString();
    }

    /**
     * Helper function to load all of the AudioCD Encoders from libraries.
     * Uses QLibraryInfo to find where libraries could be, opens all of the ones
     * that we might own audiocd_encoder_* and then uses the symbol
     * create_audiocd_encoders to obtain the encoders from that library.
     * @param worker Worker needed if the plugin is going to be used to encode
     * something.
     * @param encoders container for new encoders.
     */
    static void findAllPlugins(KIO::WorkerBase *worker, QList<AudioCDEncoder *> &encoders);

protected:
    /**
     * Pointer to the ioWorker that is running this encoder.
     * Used (only?) for the data() function to pass back encoded data.
     */
    KIO::WorkerBase *ioWorker;
};

#endif // AUDIOCD_ENCODER_H
