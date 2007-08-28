/*

    Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
    Copyright (C) 2005 Benjamin Meyer <ben at meyerhome dot net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    Permission is also granted to link this program with the Qt
    library, treating Qt like a library that normally accompanies the
    operating system kernel, whether or not that is in fact the case.

*/

#ifndef KCMAUDIOCD_H
#define KCMAUDIOCD_H

//Added by qt3to4:
#include <Q3PtrList>
#include <kcmodule.h>
class KConfigDialogManager;

#include "ui_audiocdconfig.h"

class AudiocdConfig : public QWidget, public Ui::AudiocdConfig
{
public:
  explicit AudiocdConfig( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class KAudiocdModule : public KCModule
{
  Q_OBJECT

public:

  explicit KAudiocdModule(QWidget *parent=0, const QVariantList &args=QVariantList());
  ~KAudiocdModule();

  QString quickHelp() const;

public slots:
  void defaults();
  void save();
  void load();

private slots:
  void updateExample();
  void slotConfigChanged();
  void slotEcEnable();
  void slotModuleChanged();

private:
  KConfig *config;
  bool configChanged;

  int getBitrateIndex(int value);

  Q3PtrList<KConfigDialogManager> encoderSettings;
  AudiocdConfig *audioConfig;
};

#endif // KCMAUDIOCD_H
