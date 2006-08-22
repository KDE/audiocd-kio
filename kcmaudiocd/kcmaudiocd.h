/*

    Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    Permission is also granted to link this program with the Qt
    library, treating Qt like a library that normally accompanies the
    operating system kernel, whether or not that is in fact the case.

*/


#ifndef KAUDIOCDCONFIG_H
#define KAUDIOCDCONFIG_H

class KConfigDialogManager;

#include "audiocdconfig.h"
class KAudiocdModule : public AudiocdConfig
{
  Q_OBJECT

public:

  KAudiocdModule(QWidget *parent=0, const char *name=0);
  ~KAudiocdModule();

  QString quickHelp() const;

public slots:
  void defaults();
  void save();
  void load();
  void load(bool useDefaults);

private slots:
  void updateExample();
  void slotConfigChanged();
  void slotEcEnable();
  void slotModuleChanged();

private:
  KConfig *config;
  bool configChanged;

  int getBitrateIndex(int value);

  QPtrList<KConfigDialogManager> encoderSettings;
};

#endif // KAUDIOCDCONFIG_H

