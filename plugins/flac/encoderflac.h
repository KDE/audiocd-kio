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

#include "ui_encoderflacconfig.h"

#include <audiocdencoder.h>

class EncoderFLACConfig : public QWidget, public Ui::EncoderFLACConfig {
public:
  explicit EncoderFLACConfig(QWidget *parent = nullptr) : QWidget( parent ) { setupUi( this ); }
};

/**
 * FLAC encoder.
 * Check out https://xiph.org/flac/ for more information.
 */
class EncoderFLAC : public AudioCDEncoder {

public:
  explicit EncoderFLAC(KIO::SlaveBase *slave);
  ~EncoderFLAC();

  virtual QString type() const override { return QLatin1String( "FLAC" ); }
  virtual bool init() override;
  virtual void loadSettings() override;
  virtual unsigned long size(long time_secs) const override;
  virtual const char * fileType() const override { return "flac"; }
  virtual const char * mimeType() const override { return "audio/x-flac"; }
  virtual void fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment ) override;
  virtual long readInit(long size) override;
  virtual long read(qint16 * buf, int frames) override;
  virtual long readCleanup() override;
  virtual QWidget* getConfigureWidget(KConfigSkeleton** manager) const override;

  class Private;
private:
  Private * d;

};

#endif // ENCODER_FLAC_H

