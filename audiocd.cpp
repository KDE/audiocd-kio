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

#ifndef __GNUC__
#define __GNUC__ 1
#endif
#undef __STRICT_ANSI__
#include <asm/types.h>
#include <linux/cdrom.h>
#endif

#include <sys/ioctl.h>
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/cdio.h>
#endif

  void paranoiaCallback(long, int);
  int kdemain(int argc, char ** argv);
#ifndef CDPARANOIA_STATIC
  int FixupTOC(cdrom_drive *d, int tracks);
#endif

}

#include "audiocd.h"

#include "encoderlame.h"
#include "encoderwav.h"
#include "encodervorbis.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <kmacroexpander.h>
#include <qfile.h>
#include <kdebug.h>
#include <kprotocolmanager.h>
#include <klocale.h>
// CDDB stuff
#include <client.h>

using namespace KIO;

#define QFL1(x) QString::fromLatin1(x)
#define DEFAULT_CD_DEVICE "/dev/cdrom"

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
  // KApplication is used as libkcddb uses ioslaves which need a valid
  // kapp pointer
  KApplication app(argc, argv, "kio_audiocd", false, true);

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

enum Which_dir { Unknown = 0, // Error 
	         Device, // Show which device it is?
		 ByTrack, // Always static Track %X file
		 Title, // Folder with the name of the album so users can copy
		 Info, // CDDB info |TODO implement|
		 Root, // The root directory, shows all these :)
		 FullCD, // Show a single file containing all of the data
                 EncoderDir // A directory created by an encoder
	       };

class AudioCDProtocol::Private
{
  public:

    Private()
    {
      clearargs();
      discid = 0;
      based_on_cddb = false;
      s_bytrack = i18n("By Track");
      s_track = i18n("Track %1");
      s_info = i18n("Information");
      s_fullCD = i18n("Full CD");
    }

    void clearargs()
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
    QStringList track_titles;
    QStringList titles;
    int cd_year;
    bool is_audio[100];
    bool based_on_cddb;
    QString s_bytrack;
    QString s_track;
    QString s_info;
    QString s_fullCD;

    Which_dir which_dir;
    int encoder_dir_type;
    /**
     * Do we want to rip all
     * tracks in one big file?
     */
    bool req_allTracks;
    int req_track;
    QString fname;
    QString fileNameTemplate;
};

// These are the only garenteed encoders to be built, the rest
// are dynamic and thus are little more then numbers.
#define FileTypeCDA 1
#define FileTypeWAV 2

AudioCDProtocol::AudioCDProtocol (const QCString & pool, const QCString & app)
  : SlaveBase("audiocd", pool, app)
{
  d = new Private;
  
  // Add encoders
  Encoder *lame = new EncoderLame(this);
  if ( ! lame->init() ){
    delete lame;
    lame = NULL;
  }
  else
    encoders.insert(4, lame);
#ifdef HAVE_VORBIS
  Encoder *vorbis = new EncoderVorbis(this);
  encoders.insert(3, vorbis);
#endif
  Encoder *wav = new EncoderWav(this);
  encoders.insert(FileTypeWAV, wav);
  Encoder *cda = new EncoderCda(this);
  encoders.insert(FileTypeCDA, cda);
}

AudioCDProtocol::~AudioCDProtocol()
{
  delete d;
  
  QMap<int, Encoder*>::Iterator it;
  for ( it = encoders.begin(); it != encoders.end(); ++it ) {
    //qDebug("Deleteing %s", it.data()->type().latin1());
    delete it.data();
    encoders.remove(it);
  }
}

QString AudioCDProtocol::extension(int encoder)
{
  if(encoders.contains(encoder))
    return QString(".")+encoders[encoder]->fileType();

  Q_ASSERT(false);
  return QString::fromLatin1("");
}

int AudioCDProtocol::encoderFromExtension(const QString& extension)
{
  QMap<int, Encoder*>::Iterator it;
  for ( it = encoders.begin(); it != encoders.end(); ++it ) {
    if(QString(".")+it.data()->fileType() == extension)
      return it.key();
  }
  Q_ASSERT(false);
  return -1;
}

int AudioCDProtocol::determineEncoder(const QString & filename)
{
  int len = filename.length();
  int pos = filename.findRev('.');
  return encoderFromExtension(filename.right(len - pos));
}

struct cdrom_drive *
AudioCDProtocol::initRequest(const KURL & url)
{
  if (url.hasHost())
  {
    error(KIO::ERR_UNSUPPORTED_ACTION,
	  i18n("You cannot specify a host with this protocol. "
	       "Please use the audiocd:/ format instead."));
    return 0;
  }

  // Tell encoders to read their settings.
  QMap<int, Encoder*>::Iterator it;
  for ( it = encoders.begin(); it != encoders.end(); ++it ) {
    it.data()->init();
  }

  // First get the parameters (settings).
  getParameters();

  // Then these parameters can be overruled by args in the URL.
  parseURLArgs(url);

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
  // See if it is an encoder directory
  for ( it = encoders.begin(); it != encoders.end(); ++it ) {
    if(it.data()->type() == d->fname){
      dname = d->fname;
      d->fname = "";
      break;
    }
  }
  // Other Hard coded directories
  if (dname.isEmpty() &&
      (d->fname == d->cd_title  ||
       d->fname == d->s_bytrack || d->fname == d->s_info ||
       d->fname == QFL1("By Track") ||
       d->fname == QFL1("Information") ||
       d->fname == d->s_fullCD || d->fname == QFL1("dev")))
    {
      dname = d->fname;
      d->fname = "";
    }
  /** end hack */
  
  // See if they want a directory
  bool encoderdir = false;
  for ( it = encoders.begin(); it != encoders.end(); ++it ) {
    if(it.data()->type() == dname){
      d->which_dir = EncoderDir;
      d->encoder_dir_type = it.key();
      encoderdir = true;
      break;
    }
  }
 
  if (!encoderdir)
  if (dname.isEmpty())
    d->which_dir = Root;
  else if (dname == d->cd_title)
    d->which_dir = Title;
  else if (dname == d->s_bytrack || dname == QFL1("By Track"))
    d->which_dir = ByTrack;
  else if (dname == d->s_info || dname == QFL1("Information"))
    d->which_dir = Info;
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

  // See if the url is a track
  d->req_track = -1;
  if (!d->fname.isEmpty()){
    QString name(d->fname);
    
    // Remove extension
    int dot = name.findRev('.');
    if (dot >= 0)
      name.truncate(dot);
    
    // See if it matches a cddb title
    int trackNumber;
    for (trackNumber = 0; trackNumber < d->tracks; trackNumber++){
      if (d->titles[trackNumber] == name)
        break;
    }
    if (trackNumber < d->tracks)
      d->req_track = trackNumber;
    else {
      /* Not found in title list.  Try hard to find a number in the
         string.  */
      unsigned int start = 0;
      unsigned int end = 0;
      // Find where the numbers start
      while (start < name.length()){
        if (name[start++].isDigit())
          break;
      }
      // Find where the numbers end
      for (end = start; end < name.length(); end++)
        if (!name[end].isDigit())
          break;
      if (start < name.length()){
        bool ok;
        // The external representation counts from 1 so subtrac 1.
        d->req_track = name.mid(start-1, end - start+2).toInt(&ok) - 1;
        if (!ok)
          d->req_track = -1;
      }
    }
  }
  if (d->req_track >= d->tracks)
    d->req_track = -1;

  // Are we in the directory that lists "full CD" files?
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

void AudioCDProtocol::get(const KURL & url)
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

 int filetype = determineEncoder(d->fname);

 if(d->based_on_cddb){
    QString trackName;
    // do we rip the whole CD?
    if (d->req_allTracks)
      // YES => the title of the file is the title of the CD
      trackName = d->cd_title.utf8().data();
    else
      // NO => title of the track.
      trackName = d->track_titles[d->req_track];

    if(encoders.contains(filetype))
      encoders[filetype]->fillSongInfo(trackName, d->cd_artist, d->cd_title, d->cd_category, d->req_track+1, d->cd_year);
  }
  long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector);
  long time_secs      = (8 * totalByteCount) / (44100 * 2 * 16);

  if(encoders.contains(filetype)){
     totalSize(encoders[filetype]->size(time_secs));
     emit mimeType(QFL1(encoders[filetype]->mimeType()));
  }
  
  // Read data (track/disk) from the cd 
  paranoiaRead(drive, firstSector, lastSector, filetype);

  // send an empty QByteArray to signal end of data.
  data(QByteArray());

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
  atom.m_str = url.fileName().replace('/', QFL1("%2F"));
  kdDebug(7117) << k_funcinfo << atom.m_str << endl;
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
      int fileType = determineEncoder(d->fname);
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
  d->track_titles.clear();
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
      KCDDB::CDInfo info = c.bestLookupResponse();
      d->cd_title = info.title;
      d->cd_artist = info.artist;
      d->cd_category = info.genre;
      d->cd_year = info.year;

      KCDDB::TrackInfoList t = info.trackInfoList;
      for (uint i = 0; i < t.count(); i++)
        {
          QString n;
          n.sprintf("%02d", i + 1);

	  QMap<QChar, QString> macros;
	  macros['A'] = d->cd_artist;
	  macros['a'] = d->cd_title;
	  macros['t'] = t[i].title;
	  macros['n'] = n;
	  macros['g'] = d->cd_category;
	  macros['y'] = QString::number(d->cd_year);

	  QString title = KMacroExpander::expandMacros(d->fileNameTemplate, macros, '%').replace('/', QFL1("%2F"));
          d->titles.append(title);
	  d->track_titles.append(t[i].title);
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

  /* TODO We can't handle which_dir == Device for now */

  bool do_tracks = true;

  if (d->which_dir == Root)
    {
      /* List our virtual directories.  */
      app_dir(entry, d->s_info, 1);
      listEntry(entry, false);
      app_dir(entry, d->cd_title, d->tracks);
      listEntry(entry, false);
      app_dir(entry, d->s_bytrack, d->tracks);
      listEntry(entry, false);
      app_dir(entry, QFL1("dev"), 1);
      listEntry(entry, false);

      // add the directory for "fullCD" files
      int numberOfFullCDFiles = 1;
      
      QMap<int, Encoder*>::Iterator it;
      for ( it = encoders.begin(); it != encoders.end(); ++it ) {
        QString name = it.data()->type();
	//qDebug("Adding %s to virtual directory", name.latin1());
	if(!name.isEmpty()){
	  app_dir(entry, name, d->tracks);
          listEntry(entry, false);
          numberOfFullCDFiles++;
        }
      }
	
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
  else if (d->which_dir == Info){
    // TODO List some text file CDDB file perhaps?
    do_tracks = false;
  }

  if (do_tracks)
  {
    QString fullCDTrack(QFL1("Full CD"));

    // if we're listing the "full CD" subdirectory :
    if ( (d->which_dir == FullCD) ) {
      QMap<int, Encoder*>::Iterator it;
      for ( it = encoders.begin(); it != encoders.end(); ++it ) {
        addEntry(fullCDTrack, it.key(), drive, -1);
        if (d->based_on_cddb)
          addEntry(d->cd_title, it.key(), drive, -1);
      }
    }
    else
    { // listing another dir than the "FullCD" one.
      for (int trackNumber = 1; trackNumber <= d->tracks; trackNumber++)
      {
        if (!d->is_audio[trackNumber-1])
          continue;

        switch (d->which_dir) {
          case Device:
          case Root:{
            QString name;
	    name.sprintf("track%02d", trackNumber);
            addEntry(name, FileTypeCDA, drive, trackNumber);
            break;
          }
          case ByTrack:{
            QString num2;
            num2.sprintf("%02d", trackNumber);
            addEntry(d->s_track.arg(num2), FileTypeWAV, drive, trackNumber);
            break;
          }
          case Title:
            addEntry(d->titles[trackNumber - 1],
			    FileTypeWAV, drive, trackNumber);
            break;
              
	  case EncoderDir:
	    addEntry(d->titles[trackNumber - 1],
			    d->encoder_dir_type, drive, trackNumber);
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
  totalSize(entry.count());
  listEntry(entry, true);

  cdda_close(drive);

  finished();
}

  void
AudioCDProtocol::addEntry(const QString& trackTitle, int encoder, struct cdrom_drive * drive, int trackNo)
{
  // now we check if we were compiled with or without
  // ogg/mp3 support. we will not add the file if it's
  // of type ogg/mp3 and that we were not compiled with
  // support of those formats.
  // => less work/less possibility of mistake/less #ifdef
  // ugliness for the caller.
  QMap<int, Encoder*>::Iterator it;
  bool found = false;
  for ( it = encoders.begin(); it != encoders.end(); ++it ) {
    if( it.key() == encoder )
      found = true;
  }
  if(!found){
    //qDebug("Unable to find encoder for %s", extension(encoder).latin1());
    return;
  }
  
  long theFileSize = 0;
  if (trackNo == -1)
  { // adding entry for the full CD
    theFileSize = fileSize(cdda_track_firstsector(drive, 1),
			   cdda_track_lastsector(drive, cdda_tracks(drive)),
			   encoder);
  }
  else
  { // adding one regular track
    long firstSector    = cdda_track_firstsector(drive, trackNo);
    long lastSector     = cdda_track_lastsector(drive, trackNo);
    theFileSize = fileSize(firstSector, lastSector, encoder);
  }
  UDSEntry entry;
  app_file(entry, trackTitle + extension(encoder), theFileSize);
  listEntry(entry, false);
}

long
AudioCDProtocol::fileSize(struct cdrom_drive* drive, int trackNumber,
		int encoder)
{
  return fileSize(cdda_track_firstsector(drive, trackNumber),
                    cdda_track_lastsector(drive, trackNumber),
                    encoder);
}

long
AudioCDProtocol::fileSize(long firstSector, long lastSector, int encoder)
{
  if(!encoders.contains(encoder))
    return 0;
  
  long filesize = CD_FRAMESIZE_RAW * ( lastSector - firstSector );
  long length_seconds = (filesize) / 176400;

  return encoders[encoder]->size(length_seconds);
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
AudioCDProtocol::parseURLArgs(const KURL & url)
{
  d->clearargs();

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
      d->path = value;
    else if (attribute == QFL1("paranoia_level"))
      d->paranoiaLevel = value.toInt();
  }
}

  void
AudioCDProtocol::paranoiaRead(
    struct cdrom_drive * drive,
    long firstSector,
    long lastSector,
    int encoderId
)
{
  // Because of the nice big while loop calculate this here.
  if(!encoders.contains(encoderId))
    return;
  Encoder *encoder = encoders[encoderId];
	
  cdrom_paranoia * paranoia = paranoia_init(drive);
  if (0 == paranoia) {
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

  long processed(0);
  long currentSector(firstSector);

  processed += encoder->readInit(CD_FRAMESIZE_RAW * (lastSector - firstSector));
  processedSize(processed);
  
  while (currentSector < lastSector)
  {
    int16_t * buf = paranoia_read(paranoia, paranoiaCallback);
    if (0 == buf) {
      kdDebug(7117) << "Unrecoverable error in paranoia_read" << endl;
      break;
    }
    
    ++currentSector;

    int encoderProcessed=encoder->read(buf, CD_FRAMESAMPLES);
    if(encoderProcessed == -1)
      break;
    processed += encoderProcessed;

    processedSize(processed);
  }

  processed += encoder->readCleanup();
  processedSize(processed);

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

  config->setGroup("FileName");
  d->fileNameTemplate = config->readEntry("file_name_template", "%n %t");

  // Tell the encoders to load their settings
  QMap<int, Encoder*>::Iterator it;
  for ( it = encoders.begin(); it != encoders.end(); ++it )
    it.data()->getParameters(config);

  delete config;
}

void paranoiaCallback(long, int)
{
  // Do we want to show info somewhere ?
  // Not yet.
  // Why not?
}

// vim:ts=2:sw=2:tw=78:et:
