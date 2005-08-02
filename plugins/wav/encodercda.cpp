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

#include "encodercda.h"

class EncoderCda::Private
{
  public:
  
};

unsigned long EncoderCda::size(long time_secs) const {
  //return (time_secs *   (44100 * 2 * 16))/8;
  return (time_secs) * 176400;
}

const char * EncoderCda::mimeType() const {
  return "audio/x-cda";
}

// Remove this by calculating CD_FRAMESIZE_RAW from the frames
extern "C"
{
  #include <cdda_interface.h>
}

inline int16_t swap16 (int16_t i)
{
  return (((i >> 8) & 0xFF) | ((i << 8) & 0xFF00));
}

long EncoderCda::read(int16_t * buf, int frames){ 
  QByteArray output;
  int16_t i16 = 1;
  /* WAV is defined to be little endian, so we need to swap it
     on big endian platforms.  */
  if (((char*)&i16)[0] == 0)
    for (int i=0; i < 2 * frames; i++)
       buf[i] = swap16 (buf[i]);
  char * cbuf = reinterpret_cast<char *>(buf);
  output.setRawData(cbuf, CD_FRAMESIZE_RAW);
  ioslave->data(output);
  output.resetRawData(cbuf, CD_FRAMESIZE_RAW);
  return CD_FRAMESIZE_RAW;
}

