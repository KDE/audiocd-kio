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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef ENCODER_VORBIS_H
#define ENCODER_VORBIS_H

#include "ui_encodervorbisconfig.h"

#include <audiocdencoder.h>

class EncoderVorbisConfig : public QWidget, public Ui::EncoderVorbisConfig
{
public:
  EncoderVorbisConfig( QWidget *parent = nullptr ) : QWidget( parent ) {
    setupUi( this );
  }
};


/**
 * Ogg Vorbis encoder.
 * Check out https://xiph.org/vorbis/ for more information.
 */
class EncoderVorbis : public AudioCDEncoder {

public:
  explicit EncoderVorbis(KIO::SlaveBase *slave);
  ~EncoderVorbis();

  virtual QString type() const override { return QLatin1String( "Ogg Vorbis" ); }
  virtual bool init() override;
  virtual void loadSettings() override;
  virtual unsigned long size(long time_secs) const override;
  virtual const char * fileType() const override { return "ogg"; }
  virtual const char * mimeType() const override;
  virtual void fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment ) override;
  virtual long readInit(long size) override;
  virtual long read(qint16 * buf, int frames) override;
  virtual long readCleanup() override;
  virtual QWidget* getConfigureWidget(KConfigSkeleton** manager) const override;

private:
  long flush_vorbis();

  class Private;
  Private * d;

};

#endif // ENCODER_VORBIS_H

