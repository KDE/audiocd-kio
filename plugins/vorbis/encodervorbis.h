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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef ENCODER_VORBIS_H
#define ENCODER_VORBIS_H

#include <config.h>

#ifdef HAVE_VORBIS

#include <audiocdencoder.h>

/**
 * Ogg Vorbis encoder.
 * This encoder is only enabled when HAVE_VORBIS is set.
 * Check out http://www.vorbis.com/ for lots of information.
 */ 
class EncoderVorbis : public AudioCDEncoder {

public:
  EncoderVorbis(KIO::SlaveBase *slave);
  ~EncoderVorbis();
  
  virtual QString type() const { return "Ogg Vorbis"; };
  virtual bool init();
  virtual void loadSettings();
  virtual unsigned long size(long time_secs) const;
  virtual const char * fileType() const { return "ogg"; };
  virtual const char * mimeType() const;
  virtual void fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment );
  virtual long readInit(long size);
  virtual long read(int16_t * buf, int frames);
  virtual long readCleanup();
  virtual QWidget* getConfigureWidget(KConfigSkeleton** manager) const;

private:
  long flush_vorbis();
  
  class Private;
  Private * d;
    
};

#endif // HAVE_VORBIS

#endif // ENCODER_VORBIS_H

