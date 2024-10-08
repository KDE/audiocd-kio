/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000, 2001, 2002 Michael Matz <matz@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
  Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
  Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>
  Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
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

#ifndef ENCODER_WAV_H
#define ENCODER_WAV_H

#include "encodercda.h"
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Wav audio "encoder"
 * Takes the CDA take and adds the standard wav header.
 */
class EncoderWav : public EncoderCda
{
public:
    explicit EncoderWav(KIO::WorkerBase *worker)
        : EncoderCda(worker)
    {
    }
    ~EncoderWav() override
    {
    }
    bool init() override
    {
        return true;
    }
    unsigned long size(long time_secs) const override;
    QString type() const override
    {
        return QLatin1String("Wav");
    }
    const char *fileType() const override
    {
        return "wav";
    }
    const char *mimeType() const override;
    void fillSongInfo(KCDDB::CDInfo, int, const QString &) override
    {
    }
    long readInit(long size) override;
};

#endif // ENCODER_WAV_H
