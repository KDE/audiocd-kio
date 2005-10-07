/*
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

#include <audiocdencoder.h>
#include <klibloader.h>
#include <kdebug.h>
#include <qdir.h>
#include <qregexp.h>
#include <kstandarddirs.h> 

/**
 * Attempt to load a plugin and see if it has the symbol create_audiocd_encoders.
 * @param libFileName file to try to load.
 * @returns pointer to the symbol or NULL
 */  
void* loadPlugin( const QString &libFileName ){
#ifdef DEBUG
  kdDebug(7117) << "Trying to load library. File: \"" << libFileName.latin1() << "\"." << endl;
#endif  
  KLibLoader *loader = KLibLoader::self(); 
  if( !loader ) return NULL;
#ifdef DEBUG
  kdDebug(7117) << "We have a loader. File:  \"" << libFileName.latin1() << "\"." << endl;
#endif  
  KLibrary *lib = loader->library(libFileName.latin1());
  if( !lib ) return NULL;
#ifdef DEBUG
  kdDebug(7117) << "We have a library. File: \"" << libFileName.latin1() << "\"." << endl;
#endif
  void *cplugin = lib->symbol("create_audiocd_encoders");
  if( !cplugin ) return NULL;
#ifdef DEBUG
  kdDebug(7117) << "We have a plugin. File:  \"" << libFileName.latin1() << "\"." << endl;
#endif
  return cplugin;
}

/**
 * There might be a "better" way of doing this, but I don't know it,
 * but I do know that this does work. :)  Feel free to improve the loading system,
 * there isn't much code anyway.
 */
void AudioCDEncoder::find_all_plugins(KIO::SlaveBase *slave, QPtrList<AudioCDEncoder> &encoders){
  KStandardDirs da;

  QStringList fonts = da.findDirs("module", "");
  for ( QStringList::Iterator it = fonts.begin(); it != fonts.end(); ++it ) {
    QDir d(*it);
    if(!d.exists()){
      kdDebug(7117) << "Directory given by KStandardDirs: " << d.path().latin1() << " doesn't exists!" << endl;
      continue;
    }
    d.setFilter( QDir::Files | QDir::Hidden );
  
    QStringList list = d.entryList( "libaudiocd_encoder_*.so");
kdDebug() << "list " << list << endl;
    for (QStringList::ConstIterator it2 = list.begin(); it2 != list.end(); ++it2) 
    {
        QString fileName = *it2;
	fileName = fileName.mid(0, fileName.find('.'));
        void *function = loadPlugin(fileName);
        if(function){
          void (*functionPointer)(KIO::SlaveBase *, QPtrList<AudioCDEncoder> &) = (void (*)(KIO::SlaveBase *slave, QPtrList<AudioCDEncoder> &encoders)) function;
          functionPointer(slave, encoders);
        }
    } 
  }
}

