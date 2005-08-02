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

#ifndef ENCODER_WAV_H
#define ENCODER_WAV_H

#include "encodercda.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

/**
 * Wav audio "encoder"
 * Takes the CDA take and adds the standard wav header.
 */ 
class EncoderWav : public EncoderCda {

public:
  EncoderWav(KIO::SlaveBase *slave) : EncoderCda(slave) {};
  ~EncoderWav(){};
  virtual bool init(){ return true; };
  virtual unsigned long size(long time_secs) const;
  virtual QString type() const { return "Wav"; };
  virtual const char * fileType() const { return "wav"; };
  virtual const char * mimeType() const;
  virtual void fillSongInfo( KCDDB::CDInfo, int, const QString &){};
  virtual long readInit(long size);
  
private:
  class Private;
  Private * d;
    
};

#endif // ENCODER_WAV_H
