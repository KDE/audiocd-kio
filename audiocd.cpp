/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000, 2001, 2002 Michael Matz <matz@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
  Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
  Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <qfile.h>
#include <qstrlist.h>
#include <qdatetime.h>
#include <qregexp.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

#define QFL1(x) QString::fromLatin1(x)

extern "C"
{
#include <cdda_interface.h>
#include <cdda_paranoia.h>

/* This is in support for the Mega Hack, if cdparanoia ever is fixed, or we
   use another ripping library we can remove this.  */
#ifdef __linux__
#ifdef __KCC
/* KAI C++'s sys/types.h defines _I386_TYPES_H to get rid of non-ANSI things
   from asm/types.h, which also removes __u8,__u16, which are required
   by linux/cdrom.h.  The only chance is to undef it again, so it gets
   included.  (matz) */
#undef _I386_TYPES_H
/* And <linux/byteorder/swab.h> uses GNU C extensions without providing
   fallbacks for non-GNUisms.  Fortunately we (or other headers) don't
   need the swab routines.  */
#define _LINUX_BYTEORDER_SWAB_H
#endif
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)
typedef unsigned long long __u64;
#endif
#include <linux/cdrom.h>
#endif
#include <sys/ioctl.h>
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/cdio.h>
#endif

#ifdef HAVE_LAME
#include <lame/lame.h>
#endif

#ifdef HAVE_VORBIS
#include <time.h>
#include <vorbis/vorbisenc.h>
#endif

void paranoiaCallback(long, int);
}
#include <kconfig.h>
#include <kdebug.h>
#include <kurl.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <klocale.h>

#include "audiocd.h"
#include "client.h"

using namespace KIO;

#define MAX_IPC_SIZE (1024*32)

#define DEFAULT_CD_DEVICE "/dev/cdrom"

extern "C"
{
  int kdemain(int argc, char ** argv);
#ifndef CDPARANOIA_STATIC
  int FixupTOC(cdrom_drive *d, int tracks);
#endif

#ifdef HAVE_LAME
  static int _lamelibMissing = false;

  static lame_global_flags* (*_lamelib_lame_init)(void) = NULL;
  static int (*_lamelib_lame_init_params) (lame_global_flags*) = NULL;
  static void (*_lamelib_id3tag_init)(lame_global_flags*) = NULL;
  static void (*_lamelib_id3tag_set_album)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_artist)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_title)(lame_global_flags*, const char*) = NULL;
  static void (*_lamelib_id3tag_set_track)(lame_global_flags*, const char*) = NULL;
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
#endif
}

int start_of_first_data_as_in_toc;
int hack_track;

/* Only do this if we have shared libcdda_cdparanoia.  */
#ifndef CDPARANOIA_STATIC
/* Mega hack.  This function comes from libcdda_interface, and is called by
   it.  We need to override it, so we implement it ourself in the hope, that
   shared lib semantics make the calls in libcdda_interface to FixupTOC end
   up here, instead of it's own copy.  This usually works.
   You don't want to know the reason for this.  (matz) */
int FixupTOC(cdrom_drive *d, int tracks)
{
  int j;
  for (j = 0; j < tracks; j++) {
    if (d->disc_toc[j].dwStartSector < 0)
      d->disc_toc[j].dwStartSector = 0;
    if (j < tracks-1
        && d->disc_toc[j].dwStartSector > d->disc_toc[j+1].dwStartSector)
      d->disc_toc[j].dwStartSector = 0;
  }
  long last = d->disc_toc[0].dwStartSector;
  for (j = 1; j < tracks; j++) {
    if (d->disc_toc[j].dwStartSector < last)
      d->disc_toc[j].dwStartSector = last;
  }
  start_of_first_data_as_in_toc = -1;
  hack_track = -1;
  if (d->ioctl_fd != -1) {
    int ms_addr;
#ifdef __linux__
    struct cdrom_multisession ms_str;
    ms_str.addr_format = CDROM_LBA;
    if (ioctl(d->ioctl_fd, CDROMMULTISESSION, &ms_str) == -1)
      return -1;
    ms_addr = ms_str.addr.lba;
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
    ms_addr = 0;   /* last session */
    if (ioctl(d->ioctl_fd, CDIOREADMSADDR, &ms_addr) == -1)
      return -1;
#endif

    if (ms_addr > 100) {
      for (j = tracks-1; j >= 0; j--)
        if (j > 0 && !IS_AUDIO(d,j) && IS_AUDIO(d,j-1)) {
          if (d->disc_toc[j].dwStartSector > ms_addr - 11400) {
            /* The next two code lines are the purpose of duplicating this
             * function, all others are an exact copy of paranoias FixupTOC().
             * The gory details: CD-Extra consist of N audio-tracks in the
             * first session and one data-track in the next session.  This
             * means, the first sector of the data track is not right behind
             * the last sector of the last audio track, so all length
             * calculation for that last audio track would be wrong.  For this
             * the start sector of the data track is adjusted (we don't need
             * the real start sector, as we don't rip that track anyway), so
             * that the last audio track end in the first session.  All well
             * and good so far.  BUT: The CDDB disc-id is based on the real
             * TOC entries so this adjustment would result in a wrong Disc-ID.
             * We can only solve this conflict, when we save the old
             * (toc-based) start sector of the data track.  Of course the
             * correct solution would be, to only adjust the _length_ of the
             * last audio track, not the start of the next track, but the
             * internal structures of cdparanoia are as they are, so the
             * length is only implicitely given.  Bloody sh*.  */
            start_of_first_data_as_in_toc = d->disc_toc[j].dwStartSector;
            hack_track = j + 1;
            d->disc_toc[j].dwStartSector = ms_addr - 11400;
          }
          break;
        }
      return 1;
    }
  }
  return 0;
}
#endif

/* libcdda returns for cdda_disc_lastsector() the last sector of the last
   _audio_ track.  How broken.  For CDDB Disc-ID we need the real last sector
   to calculate the disc length.  */
long my_last_sector(cdrom_drive *drive)
{
  return cdda_track_lastsector(drive, drive->tracks);
}

/* Stupid CDparanoia returns the first sector of the first _audio_ track
   as first disc sector.  Equally broken to the last sector.  But what is
   even more shitty is, that if it happens that the first audio track is
   the first track at all, it returns a hardcoded _zero_, whatever else
   the TOC told it.  This of course happens quite often, as usually the first
   track is audio, if there's audio at all.  And usually it even works,
   because most of the time the real TOC offset is 150 frames, which we
   accounted for in our code.  This is so unbelievable ugly.  */
long my_first_sector(cdrom_drive *drive)
{
  return cdda_track_firstsector(drive, 1);
}

using namespace AudioCD;

int
kdemain(int argc, char ** argv)
{
  KInstance instance("kio_audiocd");

  kdDebug(7117) << "Starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr,
         "Usage: kio_audiocd protocol domain-socket1 domain-socket2\n"
     );

     exit(-1);
  }

  AudioCDProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7117) << "Done" << endl;
  return 0;
}

enum Which_dir { Unknown = 0, Device, ByName, ByTrack, Title, Info, Root,
                 MP3, Vorbis, FullCD };

class AudioCDProtocol::Private
{
  public:

    Private()
    {
      clear();
      discid = 0;
      based_on_cddb = false;
      s_byname = i18n("By Name");
      s_bytrack = i18n("By Track");
      s_track = i18n("Track %1");
      s_info = i18n("Information");
      s_mp3  = "MP3";
      s_vorbis = "Ogg Vorbis";
      s_fullCD = i18n("Full CD");
    }

    void clear()
    {
      req_allTracks = false;
      which_dir = Unknown;
      req_track = -1;
    }

    QString path;
    int paranoiaLevel;
    unsigned int discid;
    int tracks;
    QString cd_title;
    QString cd_artist;
    QString cd_category;
    QStringList titles;
    int cd_year;
    bool is_audio[100];
    bool based_on_cddb;
    QString s_byname;
    QString s_bytrack;
    QString s_track;
    QString s_info;
    QString s_mp3;
    QString s_vorbis;
    QString s_fullCD;

#ifdef HAVE_LAME
  lame_global_flags *gf;
  int bitrate;
  bool write_id3;
#endif

#ifdef HAVE_VORBIS
  ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
  vorbis_comment   vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */
  bool  write_vorbis_comments;
  long vorbis_bitrate_lower;
  long vorbis_bitrate_upper;
  long vorbis_bitrate_nominal;
  int  vorbis_encode_method;
  double vorbis_quality;
  int vorbis_bitrate;
#endif

    Which_dir which_dir;
    /**
     * Do we want to rip all
     * tracks in one big file?
     */
    bool req_allTracks;
    int req_track;
    QString fname;
};

AudioCDProtocol::AudioCDProtocol (const QCString & pool, const QCString & app)
  : SlaveBase("audiocd", pool, app)
{
  d = new Private;
}

AudioCDProtocol::~AudioCDProtocol()
{
  delete d;
}

/*static*/ QString AudioCDProtocol::extension(enum AudioCDProtocol::FileType fileType)
{
  switch (fileType)
  {
    case FileTypeOggVorbis:
      return QString::fromLatin1(".ogg");
    case FileTypeMP3:
      return QString::fromLatin1(".mp3");
    case FileTypeWAV:
      return QString::fromLatin1(".wav");
    case FileTypeCDA:
      return QString::fromLatin1(".cda");
    case FileTypeUnknown:
    default:
      Q_ASSERT(false);
      break;
  };
  return QString::fromLatin1("");
}

/*static*/ AudioCDProtocol::FileType AudioCDProtocol::fileTypeFromExtension(const QString& extension)
{
  if (extension == QString::fromLatin1(".wav"))
    return FileTypeWAV;
  if (extension == QString::fromLatin1(".mp3"))
    return FileTypeMP3;
  if (extension == QString::fromLatin1(".ogg"))
    return FileTypeOggVorbis;
  if (extension == QString::fromLatin1(".cda"))
    return FileTypeCDA;
  Q_ASSERT(false);
  return FileTypeUnknown;
}

/*static*/ AudioCDProtocol::FileType AudioCDProtocol::determineFiletype(const QString & filename)
{
    int len = filename.length();
    int pos = filename.findRev('.');
    return fileTypeFromExtension(filename.right(len - pos));
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

#ifdef HAVE_LAME
bool AudioCDProtocol::initLameLib(){
   if ( _lamelib_lame_init != NULL )
      return true;

   if ( _lamelibMissing == true )  // we tried already, do not try again
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
#endif

struct cdrom_drive *
AudioCDProtocol::initRequest(const KURL & url)
{
#ifdef HAVE_LAME
  initLameLib();
#endif

#ifdef HAVE_VORBIS

  vorbis_info_init(&d->vi);
  vorbis_comment_init(&d->vc);

  vorbis_comment_add_tag
    (
     &d->vc,
     const_cast<char *>("kde-encoder"),
     const_cast<char *>("kio_audiocd")
    );

#endif

	// first get the parameters from the Kontrol Center Module
  getParameters();

	// then these parameters can be overruled by args in the URL
  parseArgs(url);

#ifdef HAVE_VORBIS

  switch (d->vorbis_encode_method) {
       case 0:
/* Support very old libvorbis by simply falling through.  */
#if HAVE_VORBIS >= 2
       vorbis_encode_init_vbr(&d->vi, 2, 44100, d->vorbis_quality/10.0);
       break;
#endif
       case 1:
       vorbis_encode_init(&d->vi, 2, 44100, d->vorbis_bitrate_upper, d->vorbis_bitrate_nominal, d->vorbis_bitrate_lower);
       break;
  }

#endif

  struct cdrom_drive * drive = pickDrive();

  if (0 == drive)
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return 0;
  }

  if (0 != cdda_open(drive))
  {
    error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
    return 0;
  }

  updateCD(drive);

  d->fname = url.filename(false);
  QString dname = url.directory(true, false);
  if (!dname.isEmpty() && dname[0] == '/')
    dname = dname.mid(1);

  /* A hack, for when konqi wants to list the directory audiocd:/Bla
     it really submits this URL, instead of audiocd:/Bla/ to us. We could
     send (in listDir) the UDS_NAME as "Bla/" for directories, but then
     konqi shows them as "Bla//" in the status line.  */
  if (dname.isEmpty() &&
      (d->fname == d->cd_title || d->fname == d->s_byname ||
       d->fname == d->s_bytrack || d->fname == d->s_info ||
       d->fname == QFL1("By Name") || d->fname == QFL1("By Track") ||
       d->fname == QFL1("Information") ||
       d->fname == d->s_mp3 || d->fname == d->s_vorbis || d->fname == d->s_fullCD || d->fname == QFL1("dev")))
    {
      dname = d->fname;
      d->fname = "";
    }

  if (dname.isEmpty())
    d->which_dir = Root;
  else if (dname == d->cd_title)
    d->which_dir = Title;
  else if (dname == d->s_byname || dname == QFL1("By Name"))
    d->which_dir = ByName;
  else if (dname == d->s_bytrack || dname == QFL1("By Track"))
    d->which_dir = ByTrack;
  else if (dname == d->s_info || dname == QFL1("Information"))
    d->which_dir = Info;
  else if (dname == d->s_mp3)
    d->which_dir = MP3;
  else if (dname == d->s_vorbis)
    d->which_dir = Vorbis;
  else if (dname == d->s_fullCD || dname == QFL1("Full CD"))
    d->which_dir = FullCD;
  else if (dname.left(4) == QFL1("dev/"))
    {
      d->which_dir = Device;
      dname = dname.mid(4);
    }
  else if (dname == QFL1("dev"))
    {
      d->which_dir = Device;
      dname = "";
    }
  else
    d->which_dir = Unknown;

  d->req_track = -1;
  if (!d->fname.isEmpty())
    {
      QString n(d->fname);
      int pi = n.findRev('.');
      if (pi >= 0)
        n.truncate(pi);
      int i;
      for (i = 0; i < d->tracks; i++)
        if (d->titles[i] == n)
          break;
      if (i < d->tracks)
        d->req_track = i;
      else
        {
          /* Not found in title list.  Try hard to find a number in the
             string.  */
          unsigned int start = 0;
          unsigned int end = 0;
          /// Find where the numbers start
          while (start < n.length())
            if (n[start++].isDigit())
              break;
          /// Find where the numbers end
          for (end = start; end < n.length(); end++)
            if (!n[end].isDigit())
              break;
          if (start < n.length()){
            bool ok;
            // The external representation counts from 1 so subtrac 1.
            d->req_track = n.mid(start-1, end - start+2).toInt(&ok) - 1;
            if (!ok)
              d->req_track = -1;
          }
        }
    }
  if (d->req_track >= d->tracks)
    d->req_track = -1;

  // are we in the directory that lists "full CD"
  // files?
  d->req_allTracks = (dname.contains(d->s_fullCD) || dname.contains(QFL1("Full CD")));

  kdDebug(7117) << "audiocd: dir=" << dname << " file=" << d->fname
    << " req_track=" << d->req_track << " which_dir=" << d->which_dir << " rip full CD?=" << d->req_allTracks << endl;
  return drive;
}

bool AudioCDProtocol::getSectorsForRequest(struct cdrom_drive * drive, long & firstSector, long & lastSector) const
{
  if (d->req_allTracks)
  { // we rip all the tracks of the CD
    firstSector = cdda_track_firstsector(drive, 1);
    lastSector  = cdda_track_lastsector(drive, cdda_tracks(drive));
  }
  else
  { // we only rip the selected track
    int trackNumber = d->req_track + 1;

    if (trackNumber <= 0 || trackNumber > cdda_tracks(drive))
      return false;
    firstSector    = cdda_track_firstsector(drive, trackNumber);
    lastSector     = cdda_track_lastsector(drive, trackNumber);
  }
  return true;
}

  void
AudioCDProtocol::get(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);
  if (!drive)
    return;

  long firstSector, lastSector;
  if (!getSectorsForRequest(drive, firstSector, lastSector))
  {
    error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

 FileType filetype = determineFiletype(d->fname);

#ifdef HAVE_LAME
  if ( initLameLib() == true ){
     if (filetype == FileTypeMP3 && d->based_on_cddb && d->write_id3) {
       /* If CDDB is used to determine the filenames, tell lame to append ID3v1 TAG to MP3 Files */
       const char *tname;
       // do we rip the whole CD?
       if (d->req_allTracks)
         // YES => the title of the file is the title of the CD
         tname = d->cd_title.latin1();
       else
         // NO => title of the track.
         tname  = d->titles[d->req_track].latin1();    // set trackname
       (_lamelib_id3tag_set_album)  (d->gf, d->cd_title.latin1());
       (_lamelib_id3tag_set_artist) (d->gf, d->cd_artist.latin1());
       (_lamelib_id3tag_set_title)  (d->gf, tname+3); // since titles has preleading tracknumbers, start at position 3
       QString tn;
       tn.sprintf("%02d", d->req_track+1);
       (_lamelib_id3tag_set_track) (d->gf, tn.latin1());
     }

     if ( (_lamelib_lame_init_params) (d->gf) < 0) { // tell lame the new parameters
       kdDebug(7117) << "lame init params failed" << endl;
       return;
     }
  };
#endif

#ifdef HAVE_VORBIS

  if (filetype == FileTypeOggVorbis && d->based_on_cddb && d->write_vorbis_comments)
  {
    QString trackName;
    // do we rip the whole CD?
    if (d->req_allTracks)
      // YES => the title of the file is the title of the CD
      trackName = d->cd_title.utf8().data();
    else
      // NO => title of the track.
      trackName = d->titles[d->req_track].mid(3);

    vorbis_comment_add_tag
      (
       &d->vc,
       const_cast<char *>("title"),
       const_cast<char *>(trackName.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vc,
       const_cast<char *>("artist"),
       const_cast<char *>(d->cd_artist.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vc,
       const_cast<char *>("album"),
       const_cast<char *>(d->cd_title.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vc,
       const_cast<char *>("genre"),
       const_cast<char *>(d->cd_category.utf8().data())
      );

    vorbis_comment_add_tag
      (
       &d->vc,
       const_cast<char *>("tracknumber"),
       const_cast<char *>(QString::number(d->req_track+1).utf8().data())
      );

     QDateTime dt = QDate(d->cd_year, 1, 1);
     vorbis_comment_add_tag
      (
       &d->vc,
       const_cast<char *>("date"),
       const_cast<char *>(dt.toString(Qt::ISODate).utf8().data())
      );
  }
#endif

  long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector);
  long time_secs      = (8 * totalByteCount) / (44100 * 2 * 16);

#ifdef HAVE_LAME
  if ( initLameLib() == true ){
    if (filetype == FileTypeMP3) {
      totalSize((time_secs * d->bitrate * 1000)/8);
      mimeType(QFL1("audio/x-mp3"));
    }
  };
#endif

#ifdef HAVE_VORBIS
  if (filetype == FileTypeOggVorbis) {
    totalSize( vorbisSize(time_secs) );
    mimeType(QFL1("application/x-ogg"));
  }
#endif

  if (filetype == FileTypeWAV) {
    totalSize(44 + totalByteCount); // Include RIFF header length.
    writeHeader(totalByteCount);    // Write RIFF header.
    mimeType(QFL1("audio/x-wav"));
  }

  if (filetype == FileTypeCDA) {
    totalSize(totalByteCount);      // CDA is raw interleaved PCM Data with SampleRate 44100 and 16 Bit res.
    mimeType(QFL1("application/x-cda"));
  }

  paranoiaRead(drive, firstSector, lastSector, filetype);

  data(QByteArray());   // send an empty QByteArray to signal end of data.

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::stat(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);
  if (!drive)
    return;

  bool isFile = !d->fname.isEmpty();

  // the track number. 0 if ripping
  // the whole CD.
  int trackNumber = d->req_track + 1;

  if (!d->req_allTracks)
  { // we only want to rip one track.
    // does this track exist?
    if (isFile && (trackNumber < 1 || trackNumber > d->tracks))
    {
      error(KIO::ERR_DOES_NOT_EXIST, url.path());
      return;
    }
  }

  UDSEntry entry;

  UDSAtom atom;
  atom.m_uds = KIO::UDS_NAME;
  atom.m_str = url.filename().replace('/', QFL1("%2F"));
  kdDebug() << k_funcinfo << atom.m_str << endl;
  entry.append(atom);

  atom.m_uds = KIO::UDS_FILE_TYPE;
  atom.m_long = isFile ? S_IFREG : S_IFDIR;
  entry.append(atom);

  atom.m_uds = KIO::UDS_ACCESS;
  atom.m_long = 0400;
  entry.append(atom);

  atom.m_uds = KIO::UDS_SIZE;
  if (!isFile)
  {
    atom.m_long = cdda_tracks(drive);
  }
  else
  {
      FileType fileType = determineFiletype(d->fname);
      long firstSector, lastSector;
      getSectorsForRequest(drive, firstSector, lastSector);
      atom.m_long = fileSize(firstSector, lastSector, fileType);
  }

  entry.append(atom);

  statEntry(entry);

  cdda_close(drive);

  finished();
}

  unsigned int
AudioCDProtocol::get_discid(struct cdrom_drive * drive)
{
  unsigned int id = 0;
  for (int i = 1; i <= drive->tracks; i++)
    {
      unsigned int n = cdda_track_firstsector (drive, i) + 150;
      if (i == hack_track)
        n = start_of_first_data_as_in_toc + 150;
      n /= 75;
      while (n > 0)
        {
          id += n % 10;
          n /= 10;
        }
    }
  unsigned int l = (my_last_sector(drive) + 1) / 75;
  l -= my_first_sector(drive) / 75;
  id = ((id % 255) << 24) | (l << 8) | drive->tracks;
  return id;
}

void
AudioCDProtocol::updateCD(struct cdrom_drive * drive)
{
  unsigned int id = get_discid(drive);
  if (id == d->discid)
    return;
  d->discid = id;
  d->tracks = cdda_tracks(drive);
  d->cd_title = i18n("No Title");
  d->titles.clear();
  KCDDB::TrackOffsetList qvl;

  for (int i = 0; i < d->tracks; i++)
    {
      d->is_audio[i] = cdda_track_audiop(drive, i + 1);
      if (i+1 != hack_track)
        qvl.append(cdda_track_firstsector(drive, i + 1) + 150);
      else
        qvl.append(start_of_first_data_as_in_toc + 150);
    }
  /* Here the + 150 isn't strictly needed, as internally both are subtracted
     so it cancels out, but for consistency we add it here too.  */
  qvl.append(my_first_sector(drive) + 150);
  qvl.append(my_last_sector(drive) + 150);

  KCDDB::Client c;

  KCDDB::CDDB::Result result = c.lookup(qvl);

  if (result == KCDDB::CDDB::Success)
    {
      d->based_on_cddb = true;
      KCDDB::CDInfo info = c.lookupResponse().first();
      d->cd_title = info.title;
      d->cd_artist = info.artist;
      d->cd_category = info.genre;
      d->cd_year = info.year;

      KCDDB::TrackInfoList t = info.trackInfoList;
      for (uint i = 0; i < t.count(); i++)
        {
          QString n;
          n.sprintf("%02d ", i + 1);
          d->titles.append (n + t[i].title);
        }
      return;
    }

  d->based_on_cddb = false;
  for (int i = 0; i < d->tracks; i++)
    {
      QString num;
      int ti = i + 1;
      QString s;
      num.sprintf("%02d", ti);
      if (cdda_track_audiop(drive, ti))
        s = d->s_track.arg(num);
      else
        s.sprintf("data%02d", ti);
      d->titles.append( s );
    }
}

static void
app_entry(UDSEntry& e, unsigned int uds, const QString& str)
{
  UDSAtom a;
  a.m_uds = uds;
  a.m_str = str;
  e.append(a);
}

static void
app_entry(UDSEntry& e, unsigned int uds, long l)
{
  UDSAtom a;
  a.m_uds = uds;
  a.m_long = l;
  e.append(a);
}

static void
app_dir(UDSEntry& e, const QString & n, size_t s)
{
  e.clear();
  app_entry(e, KIO::UDS_NAME, QFile::decodeName(n.local8Bit()));
  app_entry(e, KIO::UDS_FILE_TYPE, S_IFDIR);
  app_entry(e, KIO::UDS_ACCESS, 0400);
  app_entry(e, KIO::UDS_SIZE, s);
}

static void
app_file(UDSEntry& e, const QString & n, size_t s)
{
  e.clear();
  app_entry(e, KIO::UDS_NAME, QFile::decodeName(n.local8Bit()));
  app_entry(e, KIO::UDS_FILE_TYPE, S_IFREG);
  app_entry(e, KIO::UDS_ACCESS, 0400);
  app_entry(e, KIO::UDS_SIZE, s);
}

#ifdef HAVE_VORBIS
  long
AudioCDProtocol::vorbisSize(long time_secs)
{
  long vorbis_size;
  switch (d->vorbis_encode_method)
  {
  case 0: // quality based encoding

#if HAVE_VORBIS >= 2 // If really old Vorbis is being used, skip this nicely.

  {
    // Estimated numbers based on the Vorbis FAQ:
    // http://www.xiph.org/archives/vorbis-faq/200203/0030.html

    static long vorbis_q_bitrate[] = { 60,  74,  86,  106, 120, 152,
				       183, 207, 239, 309, 440 };
    long quality = static_cast<long>(d->vorbis_quality);
    if (quality < 0 || quality > 10)
      quality = 3;
    vorbis_size = (time_secs * vorbis_q_bitrate[quality] * 1000) / 8;

    break;
  }

#endif // HAVE_VORBIS >= 2

  default: // bitrate based encoding
    vorbis_size = (time_secs * d->vorbis_bitrate/8);
    break;
  }

  return vorbis_size;
}
#endif

  void
AudioCDProtocol::listDir(const KURL & url)
{
  struct cdrom_drive * drive = initRequest(url);
  if (!drive)
    return;

  UDSEntry entry;

  if (d->which_dir == Unknown)
    {
      error(KIO::ERR_DOES_NOT_EXIST, url.path());
      return;
    }

  if (!d->fname.isEmpty() && d->which_dir != Device)
    {
      error(KIO::ERR_IS_FILE, url.path());
      return;
    }

  /* XXX We can't handle which_dir == Device for now */

  bool do_tracks = true;

  if (d->which_dir == Root)
    {
      /* List our virtual directories.  */
      if (d->based_on_cddb)
        {
          app_dir(entry, d->s_byname, d->tracks);
          listEntry(entry, false);
        }
      app_dir(entry, d->s_info, 1);
      listEntry(entry, false);
      app_dir(entry, d->cd_title, d->tracks);
      listEntry(entry, false);
      app_dir(entry, d->s_bytrack, d->tracks);
      listEntry(entry, false);
      app_dir(entry, QFL1("dev"), 1);
      listEntry(entry, false);

#ifdef HAVE_LAME
      if ( initLameLib() == true ){
         app_dir(entry, d->s_mp3, d->tracks);
         listEntry(entry, false);
      };
#endif

#ifdef HAVE_VORBIS
      app_dir(entry, d->s_vorbis, d->tracks);
      listEntry(entry, false);
#endif

      // add the directory for "fullCD" files
      int numberOfFullCDFiles = 1 /* fullCD.wav */
      #ifdef HAVE_LAME
                              + 1 /* fullCD.mp3 */
      #endif
      #ifdef HAVE_VORBIS
                              + 1 /* fullCD.ogg */
      #endif
                              ;
      if (d->based_on_cddb)
      { // we are using CDDB. there will
        // be twice as much files (also <cd_title>.{wav,mp3,ogg})
        numberOfFullCDFiles = numberOfFullCDFiles * 2;
      }
      app_dir(entry, d->s_fullCD, numberOfFullCDFiles);
      listEntry(entry, false);
    }
  else if (d->which_dir == Device && url.path().length() <= 5) // "/dev{/}"
    {
      app_dir(entry, QFL1("cdrom"), d->tracks);
      listEntry(entry, false);
      do_tracks = false;
    }
  else if (d->which_dir == Info)
    {
      /* List some text files */
      /* XXX */
      do_tracks = false;
    }

  if (do_tracks)
  {
    QString fullCDTrack(QFL1("Full CD"));

    // if we're listing the "full CD" subdirectory :
    if ( (d->which_dir == FullCD) )
    { // add the entries for the
      // full CD.
      addEntry(fullCDTrack, FileTypeOggVorbis, drive, -1);
      addEntry(fullCDTrack, FileTypeMP3, drive, -1);
      addEntry(fullCDTrack, FileTypeWAV, drive, -1);
      if (d->based_on_cddb)
      {
        addEntry(d->cd_title, FileTypeOggVorbis, drive, -1);
        addEntry(d->cd_title, FileTypeMP3, drive, -1);
        addEntry(d->cd_title, FileTypeWAV, drive, -1);
      }
    }
    else
    { // listing another dir than the "FullCD" one.
      for (int i = 1; i <= d->tracks; i++)
      {
        if (d->is_audio[i-1])
        {

          QString s;
          /*if (i==1)
            s.sprintf("_%08x.wav", d->discid);
          else*/
          QString num2;
          num2.sprintf("%02d", i);
          QString name;

          switch (d->which_dir)
            {
              case Device:
              case Root:
                name.sprintf("track%02d", i);
                addEntry(name, FileTypeCDA, drive, i);
                break;
              case ByTrack:
                addEntry(d->s_track.arg(num2), FileTypeWAV, drive, i);
                break;
#ifdef HAVE_LAME
              case MP3:
                if ( initLameLib() == true ){
                  addEntry(d->titles[i - 1], FileTypeMP3, drive, i);
                }
                break;
#endif
              case Vorbis:
                addEntry(d->titles[i - 1], FileTypeOggVorbis, drive, i);
                break;
              case ByName:
              case Title:
                addEntry(d->titles[i - 1], FileTypeWAV, drive, i);
                break;
              case Info:
              case Unknown:
              default:
                error(KIO::ERR_INTERNAL, url.path());
                return;
            }
        }
      }
    }
  }
  totalSize(entry.count());
  listEntry(entry, true);

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::addEntry(const QString& trackTitle, enum FileType fileType, struct cdrom_drive * drive, int trackNo)
{
  // now we check if we were compiled with or without
  // ogg/mp3 support. we will not add the file if it's
  // of type ogg/mp3 and that we were not compiled with
  // support of those formats.
  // => less work/less possibility of mistake/less #ifdef
  // ugliness for the caller.
#ifndef HAVE_LAME
  if (fileType == FileTypeMP3)
    return;
#endif
#ifndef HAVE_VORBIS
  if (fileType == FileTypeOggVorbis)
    return;
#endif
  long theFileSize = 0;
  if (trackNo == -1)
  { // adding entry for the full CD
    theFileSize = fileSize(cdda_track_firstsector(drive, 1),
                                      cdda_track_lastsector(drive, cdda_tracks(drive)),
                                      fileType);
  }
  else
  { // adding one regular track
    long firstSector    = cdda_track_firstsector(drive, trackNo);
    long lastSector     = cdda_track_lastsector(drive, trackNo);
    theFileSize = fileSize(firstSector, lastSector, fileType);
  }
  UDSEntry entry;
  app_file(entry, trackTitle + extension(fileType), theFileSize);
  listEntry(entry, false);
}

  void
AudioCDProtocol::writeHeader(long byteCount)
{
  static char riffHeader[] =
  {
    0x52, 0x49, 0x46, 0x46, // 0  "AIFF"
    0x00, 0x00, 0x00, 0x00, // 4  wavSize
    0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
    0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
    0x10, 0x00, 0x00, 0x00, // 16
    0x01, 0x00, 0x02, 0x00, // 20
    0x44, 0xac, 0x00, 0x00, // 24
    0x10, 0xb1, 0x02, 0x00, // 28
    0x04, 0x00, 0x10, 0x00, // 32
    0x64, 0x61, 0x74, 0x61, // 36 "data"
    0x00, 0x00, 0x00, 0x00  // 40 byteCount
  };

  Q_INT32 wavSize(byteCount + 44 - 8);


  riffHeader[4]   = (wavSize   >> 0 ) & 0xff;
  riffHeader[5]   = (wavSize   >> 8 ) & 0xff;
  riffHeader[6]   = (wavSize   >> 16) & 0xff;
  riffHeader[7]   = (wavSize   >> 24) & 0xff;

  riffHeader[40]  = (byteCount >> 0 ) & 0xff;
  riffHeader[41]  = (byteCount >> 8 ) & 0xff;
  riffHeader[42]  = (byteCount >> 16) & 0xff;
  riffHeader[43]  = (byteCount >> 24) & 0xff;

  QByteArray output;
  output.setRawData(riffHeader, 44);
  data(output);
  output.resetRawData(riffHeader, 44);
  processedSize(44);
}

long
AudioCDProtocol::fileSize(struct cdrom_drive* drive, int trackNumber, AudioCDProtocol::FileType fileType)
{
  return fileSize(cdda_track_firstsector(drive, trackNumber),
                    cdda_track_lastsector(drive, trackNumber),
                    fileType);
}

long
AudioCDProtocol::fileSize(long firstSector, long lastSector, AudioCDProtocol::FileType filetype)
{
  long result = 0; // the value we're searching for.

  long filesize = CD_FRAMESIZE_RAW * (
        lastSector -
        firstSector
  );

  long length_seconds = (filesize) / 176400;
#ifdef HAVE_LAME
  if ( initLameLib() == true && filetype == FileTypeMP3)
      result = (length_seconds * d->bitrate*1000) / 8;
#endif

#ifdef HAVE_VORBIS
  if (filetype == FileTypeOggVorbis)
    result = vorbisSize(length_seconds);
#endif

  if (filetype == FileTypeCDA) result = filesize;

  if (filetype == FileTypeWAV) result = filesize + 44;

  return result;
}

  struct cdrom_drive *
AudioCDProtocol::pickDrive()
{
  QCString path(QFile::encodeName(d->path));

  struct cdrom_drive * drive = 0;

  if (!path.isEmpty() && path != "/")
    drive = cdda_identify(path, CDDA_MESSAGE_PRINTIT, 0);

  else
  {
    drive = cdda_find_a_cdrom(CDDA_MESSAGE_PRINTIT, 0);

    if (0 == drive)
    {
      if (QFile(QFile::decodeName(DEFAULT_CD_DEVICE)).exists())
        drive = cdda_identify(DEFAULT_CD_DEVICE, CDDA_MESSAGE_PRINTIT, 0);
    }
  }

  if (0 == drive)
  {
    kdDebug(7117) << "Can't find an audio CD" << endl;
  }

  return drive;
}

  void
AudioCDProtocol::parseArgs(const KURL & url)
{
  d->clear();

  QString query(KURL::decode_string(url.query()));

  if (query.isEmpty() || query[0] != '?')
    return;

  query = query.mid(1); // Strip leading '?'.

  QStringList tokens(QStringList::split('&', query));

  for (QStringList::ConstIterator it(tokens.begin()); it != tokens.end(); ++it)
  {
    QString token(*it);

    int equalsPos(token.find('='));

    if (-1 == equalsPos)
      continue;

    QString attribute(token.left(equalsPos));
    QString value(token.mid(equalsPos + 1));

    if (attribute == QFL1("device"))
    {
      d->path = value;
    }

    else if (attribute == QFL1("paranoia_level"))
    {
      d->paranoiaLevel = value.toInt();
    }
  }

  /* We need to recheck the CD, if the user enabled CDDB now.
     We simply reset the saved discid, which
     forces a reread of CDDB information.
     FIXME Check if libkcddb config has changed */

//  if ((old_use_cddb != d->useCDDB ))
//    d->discid = 0;

}

inline int16_t swap16 (int16_t i)
{
  return (((i >> 8) & 0xFF) | ((i << 8) & 0xFF00));
}

  void
AudioCDProtocol::paranoiaRead(
    struct cdrom_drive * drive,
    long firstSector,
    long lastSector,
    AudioCDProtocol::FileType filetype
)
{
  cdrom_paranoia * paranoia = paranoia_init(drive);

  if (0 == paranoia)
  {
    kdDebug(7117) << "paranoia_init failed" << endl;
    return;
  }

  int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

  switch (d->paranoiaLevel)
  {
    case 0:
      paranoiaLevel = PARANOIA_MODE_DISABLE;
      break;

    case 1:
      paranoiaLevel |=  PARANOIA_MODE_OVERLAP;
      paranoiaLevel &= ~PARANOIA_MODE_VERIFY;
      break;

    case 2:
      paranoiaLevel |= PARANOIA_MODE_NEVERSKIP;
    default:
      break;
  }

  paranoia_modeset(paranoia, paranoiaLevel);

  cdda_verbose_set(drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

  paranoia_seek(paranoia, firstSector, SEEK_SET);

#ifdef HAVE_LAME
#define mp3buffer_size  8000
static char mp3buffer[mp3buffer_size];
#endif

  long processed(0);
  long currentSector(firstSector);

#ifdef HAVE_VORBIS
  if (filetype == FileTypeOggVorbis) {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_init(&d->vd,&d->vi);
    vorbis_block_init(&d->vd,&d->vb);

    srand(time(NULL));
    ogg_stream_init(&d->os,rand());

    vorbis_analysis_headerout(&d->vd,&d->vc,&header,&header_comm,&header_code);

    ogg_stream_packetin(&d->os,&header);
    ogg_stream_packetin(&d->os,&header_comm);
    ogg_stream_packetin(&d->os,&header_code);

    while (int result = ogg_stream_flush(&d->os,&d->og)) {

      if (!result) break;

      QByteArray output;

      char * oggheader = reinterpret_cast<char *>(d->og.header);
      char * oggbody = reinterpret_cast<char *>(d->og.body);

      output.setRawData(oggheader, d->og.header_len);
      data(output);
      output.resetRawData(oggheader, d->og.header_len);

      output.setRawData(oggbody, d->og.body_len);
      data(output);
      output.resetRawData(oggbody, d->og.body_len);

    }
  }
#endif

  QTime timer;
  timer.start();

  while (currentSector < lastSector)
  {
    int16_t * buf = paranoia_read(paranoia, paranoiaCallback);

    if (0 == buf)
    {
      kdDebug(7117) << "Unrecoverable error in paranoia_read" << endl;
      break;
    }
    else
    {
      ++currentSector;

#ifdef HAVE_LAME
      if ( initLameLib() == true && filetype == FileTypeMP3 ){
         int mp3bytes =
           (_lamelib_lame_encode_buffer_interleaved)
            (d->gf,buf,CD_FRAMESAMPLES,(unsigned char *)mp3buffer,(int)mp3buffer_size) ;

         if (mp3bytes < 0 ) {
            kdDebug(7117) << "lame encoding failed" << endl;
            break;
         }

         if (mp3bytes > 0) {
           QByteArray output;

           output.setRawData(mp3buffer, mp3bytes);
           data(output);
           output.resetRawData(mp3buffer, mp3bytes);
           processed += mp3bytes;
         }
      }
#endif

#ifdef HAVE_VORBIS
      if (filetype == FileTypeOggVorbis) {
        int i;
        float **buffer=vorbis_analysis_buffer(&d->vd,CD_FRAMESAMPLES);

        /* uninterleave samples */
        for(i=0;i<CD_FRAMESAMPLES;i++){
          buffer[0][i]=buf[2*i]/32768.0;
          buffer[1][i]=buf[2*i+1]/32768.0;
        }

        vorbis_analysis_wrote(&d->vd,i);

        while(vorbis_analysis_blockout(&d->vd,&d->vb)==1) {
/* Support ancient libvorbis (< RC3).  */
#if HAVE_VORBIS >= 2
	  vorbis_analysis(&d->vb,NULL);
          /* Non-ancient case.  */
          vorbis_bitrate_addblock(&d->vb);

          while(vorbis_bitrate_flushpacket(&d->vd, &d->op)) {
#else
          vorbis_analysis(&d->vb,&d->op);
          /* Make a lexical block to place the #ifdef's nearby.  */
          if (1) {
#endif
          ogg_stream_packetin(&d->os,&d->op);

          while(int result=ogg_stream_pageout(&d->os,&d->og)) {

            if (!result) break;

            QByteArray output;

            char * oggheader = reinterpret_cast<char *>(d->og.header);
            char * oggbody = reinterpret_cast<char *>(d->og.body);

	    output.setRawData(oggheader, d->og.header_len);
            data(output);
            output.resetRawData(oggheader, d->og.header_len);

            output.setRawData(oggbody, d->og.body_len);
            data(output);
            output.resetRawData(oggbody, d->og.body_len);
	    processed +=  d->og.header_len + d->og.body_len;
          }
        }
      }
      }
#endif

      if (filetype == FileTypeWAV || filetype == FileTypeCDA) {
        QByteArray output;
        int16_t i16 = 1;
        /* WAV is defined to be little endian, so we need to swap it
           on big endian platforms.  */
        if (((char*)&i16)[0] == 0)
          for (int i=0; i < 2 * CD_FRAMESAMPLES; i++)
            buf[i] = swap16 (buf[i]);
        char * cbuf = reinterpret_cast<char *>(buf);
        output.setRawData(cbuf, CD_FRAMESIZE_RAW);
        data(output);
        output.resetRawData(cbuf, CD_FRAMESIZE_RAW);
        processed += CD_FRAMESIZE_RAW;
      }

      processedSize(processed);
    }
  }
#ifdef HAVE_LAME
  if ( initLameLib() == true && filetype == FileTypeMP3) {
     int mp3bytes = _lamelib_lame_encode_finish(d->gf,(unsigned char *)mp3buffer,(int)mp3buffer_size);

     if (mp3bytes < 0 ) {
       kdDebug(7117) << "lame encoding failed" << endl;
     }

     if (mp3bytes > 0) {
       QByteArray output;
       output.setRawData(mp3buffer, mp3bytes);
       data(output);
       output.resetRawData(mp3buffer, mp3bytes);
     }
     // reinit lame after finish title
     d->gf = (_lamelib_lame_init)();
     (void) ((_lamelib_id3tag_init)(d->gf));
  }
#endif

#ifdef HAVE_VORBIS
  if (filetype == FileTypeOggVorbis) {
    ogg_stream_clear(&d->os);
    vorbis_block_clear(&d->vb);
    vorbis_dsp_clear(&d->vd);
    vorbis_info_clear(&d->vi);
  }
#endif

  paranoia_free(paranoia);
  paranoia = 0;
}


void AudioCDProtocol::getParameters() {

  KConfig *config;
  config = new KConfig(QFL1("kcmaudiocdrc"));

  config->setGroup(QFL1("CDDA"));

  if (!config->readBoolEntry(QFL1("autosearch"),true)) {
    d->path = config->readEntry(QFL1("device"),QFL1(DEFAULT_CD_DEVICE));
  }

  d->paranoiaLevel = 1; // enable paranoia error correction, but allow skipping

  if (config->readBoolEntry("disable_paranoia",false)) {
    d->paranoiaLevel = 0; // disable all paranoia error correction
  }

  if (config->readBoolEntry("never_skip",true)) {
    d->paranoiaLevel = 2;  // never skip on errors of the medium, should be default for high quality
  }

#ifdef HAVE_LAME
  if ( initLameLib() == true ){

     config->setGroup("MP3");

     int quality = config->readNumEntry("quality",2);

     if (quality < 0 ) quality = 0;
     if (quality > 9) quality = 9;

     int method = config->readNumEntry("encmethod",0);

     if (method == 0) {

       // Constant Bitrate Encoding
       (_lamelib_lame_set_VBR)(d->gf, vbr_off);
       (_lamelib_lame_set_brate)(d->gf,config->readNumEntry("cbrbitrate",160));
       d->bitrate = (_lamelib_lame_get_brate)(d->gf);
       (_lamelib_lame_set_quality)(d->gf, quality);

     } else {

       // Variable Bitrate Encoding

       if (config->readBoolEntry("set_vbr_avr",true)) {

         (_lamelib_lame_set_VBR)(d->gf,vbr_abr);
         (_lamelib_lame_set_VBR_mean_bitrate_kbps)(d->gf, config->readNumEntry("vbr_average_bitrate",0));

         d->bitrate = (_lamelib_lame_get_VBR_mean_bitrate_kbps)(d->gf);

       } else {

         if ((_lamelib_lame_get_VBR)(d->gf) == vbr_off) _lamelib_lame_set_VBR(d->gf, vbr_default);

         if (config->readBoolEntry("set_vbr_min",true))
     (_lamelib_lame_set_VBR_min_bitrate_kbps)(d->gf, config->readNumEntry("vbr_min_bitrate",0));
         if (config->readBoolEntry("vbr_min_hard",true))
     (_lamelib_lame_set_VBR_hard_min)(d->gf, 1);
         if (config->readBoolEntry("set_vbr_max",true))
     (_lamelib_lame_set_VBR_max_bitrate_kbps)(d->gf, config->readNumEntry("vbr_max_bitrate",0));

         d->bitrate = 128;
         (_lamelib_lame_set_VBR_q)(d->gf, quality);

       }

       if ( config->readBoolEntry("write_xing_tag",true) )
            (_lamelib_lame_set_bWriteVbrTag)(d->gf, 1);

     }

     switch (   config->readNumEntry("mode",0) ) {

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

     (_lamelib_lame_set_copyright)(d->gf, config->readBoolEntry("copyright",false));
     (_lamelib_lame_set_original)(d->gf, config->readBoolEntry("original",true));
     (_lamelib_lame_set_strict_ISO)(d->gf, config->readBoolEntry("iso",false));
     (_lamelib_lame_set_error_protection)(d->gf, config->readBoolEntry("crc",false));

     d->write_id3 = config->readBoolEntry("id3",true);

     if ( config->readBoolEntry("enable_lowpassfilter",false) ) {

       (_lamelib_lame_set_lowpassfreq)(d->gf, config->readNumEntry("lowpassfilter_freq",0));

       if (config->readBoolEntry("set_lowpassfilter_width",false)) {
         (_lamelib_lame_set_lowpasswidth)(d->gf, config->readNumEntry("lowpassfilter_width",0));
       }

     }

     if ( config->readBoolEntry("enable_highpassfilter",false) ) {

       (_lamelib_lame_set_highpassfreq)(d->gf, config->readNumEntry("highpassfilter_freq",0));

       if (config->readBoolEntry("set_highpassfilter_width",false)) {
         (_lamelib_lame_set_highpasswidth)(d->gf, config->readNumEntry("highpassfilter_width",0));
       }

     }
  };
#endif // HAVE_LAME

#ifdef HAVE_VORBIS

  config->setGroup("Vorbis");

  d->vorbis_encode_method = config->readNumEntry("encmethod", 0); // 0 for quality, 1 for managed bitrate
  d->vorbis_quality = config->readDoubleNumEntry("quality",3.0); // default quality level of 3, to match oggenc

  if ( config->readBoolEntry("set_vorbis_min_bitrate",false) ) {
    d->vorbis_bitrate_lower = config->readNumEntry("vorbis_min_bitrate",40) * 1000;
  } else {
    d->vorbis_bitrate_lower = -1;
  }

  if ( config->readBoolEntry("set_vorbis_max_bitrate",false) ) {
    d->vorbis_bitrate_upper = config->readNumEntry("vorbis_max_bitrate",350) * 1000;
  } else {
    d->vorbis_bitrate_upper = -1;
  }

  // this is such a hack!
  if ( d->vorbis_bitrate_upper != -1 && d->vorbis_bitrate_lower != -1 ) {
    d->vorbis_bitrate = 104000; // empirically determined ...?!

  } else {
    d->vorbis_bitrate = 160 * 1000;
  }

  if ( config->readBoolEntry("set_vorbis_nominal_bitrate",true) ) {
    d->vorbis_bitrate_nominal = config->readNumEntry("vorbis_nominal_bitrate",160) * 1000;
    d->vorbis_bitrate = d->vorbis_bitrate_nominal;
  } else {
    d->vorbis_bitrate_nominal = -1;
  }

  d->write_vorbis_comments = config->readBoolEntry("vorbis_comments",true);


#endif // HAVE_VORBIS
  delete config;
  return;
}

void paranoiaCallback(long, int)
{
  // Do we want to show info somewhere ?
  // Not yet.
}

// vim:ts=2:sw=2:tw=78:et:
