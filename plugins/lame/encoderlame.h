/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000, 2001, 2002 Michael Matz <matz@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
  Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
  Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>
  Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
  Copyright (C) 2004 Benjamin Meyer <ben + audiocd at meyerhome dot net>

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

#ifndef ENCODER_LAME_H
#define ENCODER_LAME_H

#include "audiocdencoder.h"
#include <klocale.h>

class KLibrary;

/**
 * MP3 encoder using the LAME encoding library.
 * Go to http://lame.sourceforge.net/ for lots of information.
 */ 
class EncoderLame : public AudioCDEncoder {

public:
  EncoderLame(KIO::SlaveBase *slave);
  ~EncoderLame();

  virtual QString type() const { return i18n("MP3"); }
  virtual bool init();
  virtual void loadSettings();
  virtual unsigned long size(long time_secs) const;
  virtual const char * fileType() const { return "mp3"; }
  virtual const char * mimeType() const;

  
  virtual void fillSongInfo(QString trackName,
		            QString cdArtist,
			    QString cdTitle,
			    QString cdCategory,
			    int trackNumber,
			    int cdYear);
  
  virtual long readInit(long size);
  virtual long read(int16_t * buf, int frames);
  virtual long readCleanup();
  
   virtual QWidget* getConfigureWidget(KConfigSkeleton** manager) const;

private:
  class Private;
  Private * d;
    
  KLibrary *_lamelib;
  
};

#endif // ENCODER_LAME_H

