/*
    Copyright (C) 2004 Allan Sandfeld Jensen <kde@carewolf.com>
    Copyright (C) 2005 Benjamin Meyer <ben at meyerhome dot net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ENCODER_FLAC_H
#define ENCODER_FLAC_H

#include <config-audiocd.h>

#ifdef HAVE_LIBFLAC

#include <audiocdencoder.h>

/**
 * FLAC encoder.
 * This encoder is only enabled when HAVE_LIBFLAC is set.
 * Check out http://flac.sourceforge.net/ for more information.
 */
class EncoderFLAC : public AudioCDEncoder {

public:
  explicit EncoderFLAC(KIO::SlaveBase *slave);
  ~EncoderFLAC();

  virtual QString type() const { return QLatin1String( "FLAC" ); };
  virtual bool init();
  virtual void loadSettings();
  virtual unsigned long size(long time_secs) const;
  virtual const char * fileType() const { return "flac"; };
  virtual const char * mimeType() const { return "audio/x-flac"; }
  virtual void fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment );
  virtual long readInit(long size);
  virtual long read(int16_t * buf, int frames);
  virtual long readCleanup();

  class Private;
private:
  Private * d;

};

#endif // HAVE_FLAC

#endif // ENCODER_FLAC_H

