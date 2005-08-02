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

#ifndef ENCODER_CDA_H
#define ENCODER_CDA_H

#include <audiocdencoder.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

/**
 * Raw cd "encoder"
 * Does little more then copy the data and make sure it is in the right
 * endian.
 */ 
class EncoderCda : public AudioCDEncoder {

public:
  EncoderCda(KIO::SlaveBase *slave) : AudioCDEncoder(slave) {};
  ~EncoderCda(){};
  virtual bool init(){ return true; };
  virtual void loadSettings(){};
  virtual unsigned long size(long time_secs) const;
  virtual QString type() const { return "CDA"; };
  virtual const char * mimeType() const;
  virtual const char * fileType() const { return "cda"; };
  virtual void fillSongInfo( KCDDB::CDInfo, int, const QString &){};
  virtual long readInit(long){ return 0; };
  virtual long read(int16_t * buf, int frames);
  virtual long readCleanup(){ return 0; };

private:
  class Private;
  Private * d;
    
};

#endif // ENCODER_CDA_H

