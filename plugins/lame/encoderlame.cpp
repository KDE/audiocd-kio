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

#include <config.h>
#include "encoderlame.h"
#include "encoderlameconfig.h"
#include "audiocd_lame_encoder.h"

#include <klibloader.h>
#include <kdebug.h>
#include <qgroupbox.h>
extern "C"
{
  void create_audiocd_encoders(KIO::SlaveBase *slave, QPtrList<AudioCDEncoder> &encoders)
  {
    AudioCDEncoder *lame = new EncoderLame(slave);
    if ( ! lame->init() ){
      delete lame;
      lame = NULL;
    }
    else
      encoders.append(lame);
  }
};

static int bitrates[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };

#ifdef HAVE_LAME
#include <lame/lame.h>
#else

/*
 * These are copied from lame.h and should allow lame support to be compiled in
 * even if the headers are not available.  This was requested by the Debian
 * packagers so that this can be compiled in and be purely a runtime dependancy.
 */

struct lame_global_flags;
typedef lame_global_flags *lame_t;

typedef enum vbr_mode_e {
  vbr_off=0,
  vbr_mt,
  vbr_rh,
  vbr_abr,
  vbr_mtrh,
  vbr_max_indicator,
  vbr_default=vbr_rh
} vbr_mode;

typedef enum MPEG_mode_e {
  STEREO = 0,
  JOINT_STEREO,
  DUAL_CHANNEL,
  MONO,
  NOT_SET,
  MAX_INDICATOR
} MPEG_mode;

#endif

#define QFL1(x) QString::fromLatin1(x)

extern "C"
{
  static int _lamelibMissing = false;

  static lame_global_flags* (*_lamelib_lame_init)(void) = NULL;
  static int (*_lamelib_lame_init_params) (lame_global_flags*) = NULL;
  static void (*_lamelib_id3tag_init)(lame_global_flags*) = NULL;
  static void (*_lamelib_id3tag_set_album)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_artist)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_title)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_track)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_year)(lame_global_flags*, const char *) = NULL;
  static int  (*_lamelib_id3tag_set_genre)(lame_global_flags *, const char *) = NULL;
  
  static int  (*_lamelib_lame_encode_buffer_interleaved) (
          lame_global_flags*, short int*, int, unsigned char*, int) = NULL;
  static int  (*_lamelib_lame_encode_finish) (
          lame_global_flags*, unsigned char*, int ) = NULL;
  static int  (*_lamelib_lame_set_VBR) ( lame_global_flags*, vbr_mode ) = NULL;
  static int  (*_lamelib_lame_get_VBR) ( lame_global_flags* ) = NULL;
  static int  (*_lamelib_lame_set_brate) ( lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_get_brate) ( lame_global_flags* ) = NULL;
  static int  (*_lamelib_lame_set_quality) ( lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_VBR_mean_bitrate_kbps) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_get_VBR_mean_bitrate_kbps) (
          lame_global_flags* ) = NULL;
  static int  (*_lamelib_lame_set_VBR_min_bitrate_kbps) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_VBR_hard_min) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_VBR_max_bitrate_kbps) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_VBR_q) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_bWriteVbrTag) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_mode) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_copyright) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_original) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_strict_ISO) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_error_protection) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_lowpassfreq) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_lowpasswidth) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_highpassfreq) (
          lame_global_flags*, int ) = NULL;
  static int  (*_lamelib_lame_set_highpasswidth) (
          lame_global_flags*, int ) = NULL;
}

class EncoderLame::Private
{
  public:
    lame_global_flags *gf;
    int bitrate;
    bool write_id3;
};

EncoderLame::EncoderLame(KIO::SlaveBase *slave) : AudioCDEncoder(slave) {
  d = new Private();
}

EncoderLame::~EncoderLame(){
  delete d;
  // TODO Does this class need to clean up the KLib stuff?
}

QWidget* EncoderLame::getConfigureWidget(KConfigSkeleton** manager) const {
  (*manager) = Settings::self();
  EncoderLameConfig *config = new EncoderLameConfig();
  config->cbr_settings->hide();
  return config;
}

#ifdef __OpenBSD__
#include <qdir.h>
#include <qstring.h>
#include <qstringlist.h>

static QString findMostRecentLib(QString dir, QString name)
{
       // Grab all shared libraries in the directory
       QString filter = "lib"+name+".so.*";
       QDir d(dir, filter);
       if (!d.exists())
               return NULL;
       QStringList l = d.entryList();

       // Find the best one
       int bestmaj = -1;
       int bestmin = -1;
       QString best = NULL;
       // where do we start
       uint s = filter.length()-1;
       for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
               QString numberpart = (*it).mid(s);
               uint endmaj = numberpart.find('.');
               if (endmaj == -1)
                       continue;
               bool ok;
               int maj = numberpart.left(endmaj).toInt(&ok);
               if (!ok)
                       continue;
               int min = numberpart.mid(endmaj+1).toInt(&ok);
               if (!ok)
                       continue;
               if (maj > bestmaj || (maj == bestmaj && min > bestmin)) {
                       bestmaj = maj;
                       bestmin = min;
                       best = (*it);
               }
       }
       if (best.isNull())
               return NULL;
       else
               return dir+"/"+best;
}
#endif


bool EncoderLame::init(){
   if ( _lamelib_lame_init != NULL )
      return true;

   if ( _lamelibMissing )  // we tried already, do not try again
      return false;

   // load the lame lib, if not done already
   KLibLoader *LameLib = KLibLoader::self();

#ifdef __OpenBSD__
   {
   QString libname = findMostRecentLib("/usr/local/lib", "mp3lame");
   if (!libname.isNull())
         _lamelib = LameLib->globalLibrary(libname.latin1());
   }
#else
   QStringList libpaths, libnames;
   libpaths << QFL1("/usr/lib/")
            << QFL1("/usr/local/lib/")
            << QString::null;

   libnames << QFL1("libmp3lame.so.0")
            << QFL1("libmp3lame.so.0.0.0")
            << QFL1("libmp3lame.so");

   for (QStringList::Iterator it = libpaths.begin();
                              it != libpaths.end();
                              ++it) {
      for (QStringList::Iterator lit = libnames.begin();
                                 lit != libnames.end();
                                 ++lit) {
         QString alib = *it+*lit;
         _lamelib = LameLib->globalLibrary(alib.latin1());
         if (_lamelib) break;
      }
      if (_lamelib) break;
   }
#endif

  if ( _lamelib == NULL ){
      _lamelibMissing = true;
      return false;
  }else{
    _lamelib_lame_init =
           (lame_t (*) (void))
           _lamelib->symbol("lame_init");
    _lamelib_id3tag_init =
           (void (*) (lame_global_flags*))
           _lamelib->symbol("id3tag_init");
    _lamelib_id3tag_set_album =
           (void (*) (lame_global_flags*, const char*))
           _lamelib->symbol("id3tag_set_album");
     _lamelib_id3tag_set_artist =
           (void (*) (lame_global_flags*, const char*))
           _lamelib->symbol("id3tag_set_artist");
     _lamelib_id3tag_set_title =
           (void (*) (lame_global_flags*, const char*))
           _lamelib->symbol("id3tag_set_title");
     _lamelib_id3tag_set_track =
           (void (*) (lame_global_flags*, const char*))
           _lamelib->symbol("id3tag_set_track");
     _lamelib_id3tag_set_year =
           (void (*) (lame_global_flags*, const char*))
           _lamelib->symbol("id3tag_set_year");
     _lamelib_id3tag_set_genre =
           (int (*) (lame_global_flags*, const char*))
           _lamelib->symbol("id3tag_set_genre");
     
     _lamelib_lame_init_params =
           (int (*) (lame_global_flags*))
           _lamelib->symbol("lame_init_params");
     _lamelib_lame_encode_buffer_interleaved =
           (int (*) (
                  lame_global_flags*, short int*, int, unsigned char*, int))
           _lamelib->symbol("lame_encode_buffer_interleaved");
     _lamelib_lame_encode_finish =
           (int (*) (lame_global_flags*, unsigned char*, int))
           _lamelib->symbol("lame_encode_finish");
     _lamelib_lame_set_VBR =
           (int (*) (lame_global_flags*, vbr_mode))
           _lamelib->symbol("lame_set_VBR");
     _lamelib_lame_get_VBR =
           (int (*) (lame_global_flags*))
           _lamelib->symbol("lame_get_VBR");
     _lamelib_lame_set_brate =
           (int (*) (lame_global_flags*, int))
           _lamelib->symbol("lame_set_brate");
     _lamelib_lame_get_brate =
           (int (*) (lame_global_flags*))
           _lamelib->symbol("lame_get_brate");
     _lamelib_lame_set_quality =
           (int (*) (lame_global_flags*, int))
           _lamelib->symbol("lame_set_quality");
     _lamelib_lame_set_VBR_mean_bitrate_kbps =
           (int (*) (lame_global_flags*, int))
           _lamelib->symbol("lame_set_VBR_mean_bitrate_kbps");
     _lamelib_lame_get_VBR_mean_bitrate_kbps =
           (int (*) ( lame_global_flags*))
           _lamelib->symbol("lame_get_VBR_mean_bitrate_kbps");
     _lamelib_lame_set_VBR_min_bitrate_kbps =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_VBR_min_bitrate_kbps");
     _lamelib_lame_set_VBR_hard_min =
           (int (*) (lame_global_flags*, int))
           _lamelib->symbol("lame_set_VBR_hard_min");
     _lamelib_lame_set_VBR_max_bitrate_kbps =
           (int (*) (
                  lame_global_flags*, int))
           _lamelib->symbol("lame_set_VBR_max_bitrate_kbps");
    _lamelib_lame_set_VBR_q =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_VBR_q");
    _lamelib_lame_set_bWriteVbrTag =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_bWriteVbrTag");
    _lamelib_lame_set_mode =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_mode");
    _lamelib_lame_set_copyright =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_copyright");
    _lamelib_lame_set_original =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_original");
    _lamelib_lame_set_strict_ISO =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_strict_ISO");
    _lamelib_lame_set_error_protection =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_error_protection");
    _lamelib_lame_set_lowpassfreq =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_lowpassfreq");
    _lamelib_lame_set_lowpasswidth =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_lowpasswidth");
    _lamelib_lame_set_highpassfreq =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_highpassfreq");
    _lamelib_lame_set_highpasswidth =
           (int (*) ( lame_global_flags*, int))
           _lamelib->symbol("lame_set_highpasswidth");

    // protecting for a crash in case of older lame lib
       if ( _lamelib_lame_init == NULL || _lamelib_id3tag_init == NULL ||
            _lamelib_id3tag_set_album == NULL ||
            _lamelib_id3tag_set_artist == NULL ||
            _lamelib_id3tag_set_title == NULL ||
            _lamelib_id3tag_set_track == NULL ||
            _lamelib_lame_init_params == NULL ||
            _lamelib_lame_encode_buffer_interleaved == NULL ||
            _lamelib_lame_set_VBR == NULL ||
            _lamelib_lame_get_VBR == NULL ||
            _lamelib_lame_set_brate == NULL ||
            _lamelib_lame_get_brate == NULL ||
            _lamelib_lame_set_quality == NULL ||
            _lamelib_lame_set_VBR_mean_bitrate_kbps == NULL ||
            _lamelib_lame_get_VBR_mean_bitrate_kbps == NULL ||
            _lamelib_lame_set_VBR_min_bitrate_kbps == NULL ||
            _lamelib_lame_set_VBR_hard_min == NULL ||
            _lamelib_lame_set_VBR_max_bitrate_kbps == NULL ||
            _lamelib_lame_set_VBR_q == NULL ||
            _lamelib_lame_set_mode == NULL ||
            _lamelib_lame_set_copyright == NULL ||
            _lamelib_lame_set_original == NULL ||
            _lamelib_lame_set_strict_ISO == NULL ||
            _lamelib_lame_set_error_protection == NULL ||
            _lamelib_lame_set_lowpassfreq == NULL ||
            _lamelib_lame_set_lowpasswidth == NULL ||
            _lamelib_lame_set_highpassfreq == NULL ||
            _lamelib_lame_set_highpasswidth == NULL
           ){
           _lamelibMissing = true;
           return false;
       }
       if ( NULL == (d->gf = (_lamelib_lame_init)()) )
       { // init the lame_global_flags structure with defaults
          _lamelibMissing = true;
          return false;
       }
   }

   (void) ((_lamelib_id3tag_init)(d->gf));
   return true;
}


void EncoderLame::loadSettings(){
  if ( !init() )
    return;

  Settings *settings = Settings::self();

  int quality = settings->quality();
  if (quality < 0 ) quality = 0;
  if (quality > 9) quality = 9;

  int method = settings->bitrate_constant() ? 0 : 1 ;
  if (method == 0) {
    // Constant Bitrate Encoding
    (_lamelib_lame_set_VBR)(d->gf, vbr_off);
    (_lamelib_lame_set_brate)(d->gf,bitrates[settings->cbr_bitrate()]);
    d->bitrate = (_lamelib_lame_get_brate)(d->gf);
    (_lamelib_lame_set_quality)(d->gf, quality);
  } else {
    // Variable Bitrate Encoding
    if (settings->vbr_average_br()) {
      (_lamelib_lame_set_VBR)(d->gf,vbr_abr);
      (_lamelib_lame_set_VBR_mean_bitrate_kbps)(d->gf, bitrates[settings->vbr_mean_brate()]);
       d->bitrate = (_lamelib_lame_get_VBR_mean_bitrate_kbps)(d->gf);
    } else {
      if ((_lamelib_lame_get_VBR)(d->gf) == vbr_off) _lamelib_lame_set_VBR(d->gf, vbr_default);
        if (settings->vbr_min_br())
          (_lamelib_lame_set_VBR_min_bitrate_kbps)(d->gf, bitrates[settings->vbr_min_brate()]);
      if (settings->vbr_min_hard())
        (_lamelib_lame_set_VBR_hard_min)(d->gf, 1);
      if (settings->vbr_max_br())
        (_lamelib_lame_set_VBR_max_bitrate_kbps)(d->gf, bitrates[settings->vbr_max_brate()]);

      d->bitrate = 128;
      (_lamelib_lame_set_VBR_q)(d->gf, quality);
    }

    if ( settings->vbr_xing_tag() )
       (_lamelib_lame_set_bWriteVbrTag)(d->gf, 1);
  }

  switch ( settings->stereo() ) {
    case 0: (_lamelib_lame_set_mode)(d->gf, STEREO);
                   break;
    case 1: (_lamelib_lame_set_mode)(d->gf, JOINT_STEREO);
                   break;
    case 2: (_lamelib_lame_set_mode)(d->gf,DUAL_CHANNEL);
                   break;
    case 3: (_lamelib_lame_set_mode)(d->gf,MONO);
                   break;
    default: (_lamelib_lame_set_mode)(d->gf,STEREO);
                   break;
  }

  (_lamelib_lame_set_copyright)(d->gf, settings->copyright());
  (_lamelib_lame_set_original)(d->gf, settings->original());
  (_lamelib_lame_set_strict_ISO)(d->gf, settings->iso());
  (_lamelib_lame_set_error_protection)(d->gf, settings->crc());

  d->write_id3 = settings->id3_tag();

  if ( settings->enable_lowpass() ) {

    (_lamelib_lame_set_lowpassfreq)(d->gf, settings->lowfilterfreq());

    if (settings->set_lpf_width()){
      (_lamelib_lame_set_lowpasswidth)(d->gf, settings->lowfilterwidth());
    }

  }

  if ( settings->enable_highpass()) {

    (_lamelib_lame_set_highpassfreq)(d->gf, settings->highfilterfreq());

    if (settings->set_hpf_width()){
         (_lamelib_lame_set_highpasswidth)(d->gf, settings->highfilterwidth());
    }
  }
}

unsigned long EncoderLame::size(long time_secs) const {
  return (time_secs * d->bitrate * 1000)/8;
}

const char * EncoderLame::mimeType() const {
  return "audio/x-mp3";
}

#define mp3buffer_size  8000
static char mp3buffer[mp3buffer_size];

long EncoderLame::read(int16_t * buf, int frames){
  if ( !init() )
    return -1;
  
  int mp3bytes = (_lamelib_lame_encode_buffer_interleaved)
    (d->gf,buf,frames,(unsigned char *)mp3buffer,(int)mp3buffer_size) ;

  if (mp3bytes < 0 ) {
    kdDebug(7117) << "lame encoding failed" << endl;
    return -1;
  }

  if (mp3bytes <= 0)
    return 0;

  QByteArray output;
  output.setRawData(mp3buffer, mp3bytes);
  ioslave->data(output);
  output.resetRawData(mp3buffer, mp3bytes);
  return mp3bytes;
}

long EncoderLame::readCleanup(){
  if ( !init() )
    return 0;
  
  int mp3bytes = _lamelib_lame_encode_finish(d->gf,(unsigned char *)mp3buffer,(int)mp3buffer_size);
  if (mp3bytes <= 0 ) {
    kdDebug(7117) << "lame encoding failed" << endl;
  }

  if (mp3bytes > 0) {
    QByteArray output;
    output.setRawData(mp3buffer, mp3bytes);
    ioslave->data(output);
    output.resetRawData(mp3buffer, mp3bytes);
  }
   
  // reinit lame after finish title
  d->gf = (_lamelib_lame_init)();
  (void) ((_lamelib_id3tag_init)(d->gf));
  return mp3bytes;
}

// TEST to make sure YEAR is being set!
void EncoderLame::fillSongInfo(QString trackName,
		            QString cdArtist,
			    QString cdTitle,
			    QString cdCategory,
			    int trackNumber,
			    int cdYear){
  if ( !init() || ! d->write_id3)
    return;
  /* If CDDB is used to determine the filenames, tell lame to append ID3v1 TAG to MP3 Files */
  (_lamelib_id3tag_set_album)  (d->gf, cdTitle.latin1());
  (_lamelib_id3tag_set_artist) (d->gf, cdArtist.latin1());
  (_lamelib_id3tag_set_title)  (d->gf, trackName.latin1());
  (_lamelib_id3tag_set_year)   (d->gf, QString("%i").arg(cdYear).latin1());
  (_lamelib_id3tag_set_genre)  (d->gf, cdCategory.latin1());
  QString tn;
  tn.sprintf("%02d", trackNumber);
  (_lamelib_id3tag_set_track) (d->gf, tn.latin1());
 
  if ( (_lamelib_lame_init_params) (d->gf) < 0) { // tell lame the new parameters
    kdDebug(7117) << "lame init params failed" << endl;
  }
}

