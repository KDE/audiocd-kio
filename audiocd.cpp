/*
 * Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
 * Copyright (C) 2000, 2001, 2002 Michael Matz <matz@kde.org>
 * Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
 * Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
 * Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>
 * Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2004, 2005 Benjamin Meyer <ben at meyerhome dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * ERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

extern "C"
{
	#include <cdda_interface.h>
	#include <cdda_paranoia.h>
	void paranoiaCallback(long, int);

	#include <kdemacros.h>
	KDE_EXPORT int kdemain(int argc, char ** argv);
}

#include "audiocd.h"
#include "plugins/audiocdencoder.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <kmacroexpander.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>
#include <qregexp.h>

// CDDB
#include <client.h>
#include "kcompactdisc.h"

using namespace KIO;
using namespace KCDDB;

#define QFL1(x) QString::fromLatin1(x)
#define DEFAULT_CD_DEVICE "/dev/cdrom"
#define CDDB_INFORMATION "CDDB Information"

using namespace AudioCD;

static const KCmdLineOptions options[] =
{
    { "+protocol", I18N_NOOP("Protocol name"), 0 },
    { "+pool", I18N_NOOP("Socket name"), 0 },
    { "+app", I18N_NOOP("Socket name"), 0 },
    KCmdLineLastOption
};

int kdemain(int argc, char ** argv)
{
	// KApplication uses libkcddb which needs a valid kapp pointer
	// GUIenabled must be true as libkcddb sometimes wants to communicate
	// with the user
	putenv(strdup("SESSION_MANAGER="));
	KApplication::disableAutoDcopRegistration();
	KCmdLineArgs::init(argc, argv, "kio_audiocd", 0, 0, 0, 0);
	KCmdLineArgs::addCmdLineOptions(options);
	KApplication app(false, true);

	kdDebug(7117) << "Starting " << getpid() << endl;

	KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
	AudioCDProtocol slave(args->arg(0), args->arg(1), args->arg(2));
	slave.dispatchLoop();

	kdDebug(7117) << "Done" << endl;
	return 0;
}

enum Which_dir {
	Unknown = 0, // Error
	Info, // CDDB info
	Root, // The root directory, shows all these :)
	FullCD, // Show a single file containing all of the data
	EncoderDir // A directory created by an encoder
};

class AudioCDProtocol::Private {
public:
	Private() : cd(KCompactDisc::Asynchronous) {
		clearURLargs();
		s_info = i18n("Information");
		s_fullCD = i18n("Full CD");
	}

	void clearURLargs() {
		req_allTracks = false;
		which_dir = Unknown;
		req_track = -1;
		cddbUserChoice = -1;
	}

	// The type/which of request
	bool req_allTracks;
	Which_dir which_dir;
	int req_track;
	QString fname;
	AudioCDEncoder *encoder_dir_type;

	// Misc settings
	QString device; // URL settable
	int paranoiaLevel; // URL settable
	bool reportErrors;

	// Directory strings, never change after init
	QString s_info;
	QString s_fullCD;

	// Current CD
	unsigned discid;
	unsigned tracks;
	bool trackIsAudio[100];
	KCompactDisc cd; // keep it around so that we don't assume the disk changed between every stat()

	// CDDB items
	KCDDB::CDDB::Result cddbResult;
	CDInfoList cddbList;
	int cddbUserChoice; // URL settable
	KCDDB::CDInfo cddbBestChoice;

	// Template for ..
	QString fileNameTemplate; // URL settable
	QString albumTemplate; // URL settable
	QString rsearch;
	QString rreplace;

	// Current strings for this CD and or cddb selection
	QStringList templateTitles;
	QString templateAlbumName;
};

int paranoia_read_limited_error;

AudioCDProtocol::AudioCDProtocol(const QCString & protocol, const QCString & pool, const QCString & app)
	: SlaveBase(protocol, pool, app)
{
	d = new Private;

	// Add encoders
	AudioCDEncoder::findAllPlugins(this, encoders);
	encoderTypeCDA = encoderFromExtension(".cda");
	encoderTypeWAV = encoderFromExtension(".wav");
	encoders.setAutoDelete(true);
}

AudioCDProtocol::~AudioCDProtocol()
{
	delete d;
}

AudioCDEncoder *AudioCDProtocol::encoderFromExtension(const QString& extension)
{
	AudioCDEncoder *encoder;
	for ( encoder = encoders.first(); encoder; encoder = encoders.next() ){
		if(QString(".")+encoder->fileType() == extension)
			return encoder;
	}
	Q_ASSERT(false);
	return NULL;
}

AudioCDEncoder *AudioCDProtocol::determineEncoder(const QString & filename)
{
	int len = filename.length();
	int pos = filename.findRev('.');
	return encoderFromExtension(filename.right(len - pos));
}

struct cdrom_drive * AudioCDProtocol::initRequest(const KURL & url)
{
	if (url.hasHost())
	{
		error(KIO::ERR_UNSUPPORTED_ACTION,
		i18n("You cannot specify a host with this protocol. "
				 "Please use the audiocd:/ format instead."));
		return 0;
	}

	// Load OUR Settings.
	loadSettings();
	// Then url parameters can overrule our settings.
	parseURLArgs(url);

	struct cdrom_drive * drive = getDrive();
	if (0 == drive)
		return 0;

	// Update our knowledge of the disc

#if defined(__linux__)
       if(drive->ioctl_device_name && drive->ioctl_device_name[0])
         d->cd.setDevice(drive->ioctl_device_name, 50, false);
       else
         d->cd.setDevice(drive->cdda_device_name, 50, false);
#else
        d->cd.setDevice(drive->cdda_device_name, 50, false);
#endif

#if 0
	// FreeBSD's cdparanoia as of january 5th 2006 has rather broken
	// support for non-SCSI devices. Although it finds ATA cdroms just
	// fine, there is no straightforward way to discover the device
	// name associated with the device, which throws the rest of audiocd
	// for a loop.
	//
	if ( !(drive->dev) || (COOKED_IOCTL == drive->interface) )
	{
		// For ATAPI devices, we have no real choice. Use the
		// user selected value, even if there is none.
		//
		kdWarning(7117) << "Found an ATAPI device, assuming it is the one specified by the user." << endl;
		d->cd.setDevice( d->device );
	}
	else
	{
		kdDebug(7117) << "Found a SCSI or ATAPICAM device." << endl;
		if ( strlen(drive->dev->device_path) > 0 )
		{
			d->cd.setDevice( drive->dev->device_path );
		}
		else
		{
			// But the device_path can be empty under some
			// circumstances, so build a representation from
			// the unit number and SCSI device name.
			//
			QString devname = QString::fromLatin1( "/dev/%1%2" )
				.arg( drive->dev->given_dev_name )
				.arg( drive->dev->given_unit_number ) ;
			kdDebug(7117) << "  Using derived name " << devname << endl;
			d->cd.setDevice( devname );
		}
	}
#endif

	if (d->cd.discId() != d->discid && d->cd.discId() != d->cd.missingDisc){
		d->discid = d->cd.discId();
		d->tracks = d->cd.tracks();
		for(uint i=0; i< d->cd.tracks(); i++)
			d->trackIsAudio[i] = d->cd.isAudio(i+1);

		KCDDB::Client c;
		d->cddbResult = c.lookup(d->cd.discSignature());
		d->cddbList = c.lookupResponse();
		d->cddbBestChoice = c.bestLookupResponse();
		generateTemplateTitles();
	}

	// Determine what file or folder that is wanted.
	d->fname = url.fileName(false);
	QString dname = url.directory(true, false);
	if (!dname.isEmpty() && dname[0] == '/')
		dname = dname.mid(1);

	// Kong issue where they send dirs as files, double check
	/* A hack, for when konqi wants to list the directory audiocd:/Bla
		 it really submits this URL, instead of audiocd:/Bla/ to us. We could
		 send (in listDir) the UDS_NAME as "Bla/" for directories, but then
		 konqi shows them as "Bla//" in the status line. */
	// See if it is an encoder directory
	AudioCDEncoder *encoder;
	for ( encoder = encoders.first(); encoder; encoder = encoders.next() ){
		if(encoder->type() == d->fname){
			dname = d->fname;
			d->fname = "";
			break;
		}
	}
	// Other Hard coded directories
	if (dname.isEmpty() && (d->fname == d->s_info || d->fname == d->s_fullCD ))
	{
		dname = d->fname;
		d->fname = "";
	}
	// end hack


	// See which directory they want
	d->which_dir = Unknown;
	for ( encoder = encoders.first(); encoder; encoder = encoders.next() ){
		if(encoder->type() == dname){
			d->which_dir = EncoderDir;
			d->encoder_dir_type = encoder;
			break;
		}
	}
	if ( Unknown == d->which_dir ){
		if (dname.isEmpty())
			d->which_dir = Root;
		else if (dname == d->s_info)
			d->which_dir = Info;
		else if (dname == d->s_fullCD)
			d->which_dir = FullCD;
	}

	// See if the url is a track
	d->req_track = -1;
	if (!d->fname.isEmpty()){
		QString name(d->fname);

		// Remove extension
		int dot = name.findRev('.');
		if (dot >= 0)
			name.truncate(dot);

		// See if it matches a cddb title
		uint trackNumber;
		for (trackNumber = 0; trackNumber < d->tracks; trackNumber++){
			if (d->templateTitles[trackNumber] == name)
				break;
		}
		if (trackNumber < d->tracks)
			d->req_track = trackNumber;
		else {
			/* Not found in title list. Try hard to find a number in the
				 string. */
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
	if (d->req_track >= (int)d->tracks)
		d->req_track = -1;

	// Are we in the directory that lists "full CD" files?
	d->req_allTracks = (dname.contains(d->s_fullCD));

	kdDebug(7117) << "dir=" << dname << " file=" << d->fname
		<< " req_track=" << d->req_track << " which_dir=" << d->which_dir << " full CD?=" << d->req_allTracks << endl;
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
		firstSector = cdda_track_firstsector(drive, trackNumber);
		lastSector = cdda_track_lastsector(drive, trackNumber);
	}
	return true;
}

void AudioCDProtocol::get(const KURL & url)
{
	struct cdrom_drive * drive = initRequest(url);
	if (!drive)
		return;

	if( d->fname.contains(i18n(CDDB_INFORMATION))){
		uint choice = 1;
		if(d->fname != QString("%1.txt").arg(i18n(CDDB_INFORMATION))){
			choice= d->fname.section('_',1,1).section('.',0,0).toInt();
		}
		uint count = 1;
		CDInfoList::iterator it;
		bool found = false;
		for ( it = d->cddbList.begin(); it != d->cddbList.end(); ++it ){
			if(count == choice){
				mimeType("text/html");
				data(QCString( (*it).toString().latin1() ));
				// send an empty QByteArray to signal end of data.
				data(QByteArray());
				finished();
				found = true;
				break;
			}
			count++;
		}
		if(!found && d->fname.contains(i18n(CDDB_INFORMATION)+":")){
			mimeType("text/html");
			//data(QCString( d->fname.latin1() ));
			// send an empty QByteArray to signal end of data.
			data(QByteArray());
			finished();
			found = true;
		}
		if( !found )
			error(KIO::ERR_DOES_NOT_EXIST, url.path());
		cdda_close(drive);
		return;
	}

	long firstSector, lastSector;
	if (!getSectorsForRequest(drive, firstSector, lastSector))
	{
		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		cdda_close(drive);
		return;
	}

	AudioCDEncoder *encoder = determineEncoder(d->fname);
	if(!encoder){
		cdda_close(drive);
		return;
	}

	KCDDB::CDInfo info;
	if(d->cddbResult == KCDDB::CDDB::Success){
		info = d->cddbBestChoice;

		int track = d->req_track;

		// hack
		// do we rip the whole CD?
		if (d->req_allTracks){
			track = 0;
			// YES => the title of the file is the title of the CD
			info.trackInfoList[track].title = info.title.utf8().data();
		}
		encoder->fillSongInfo(info, track, "");
	}
	long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector + 1);
	long time_secs = (8 * totalByteCount) / (44100 * 2 * 16);

	unsigned long size = encoder->size(time_secs);
	totalSize(size);
	emit mimeType(QFL1(encoder->mimeType()));

	// Read data (track/disk) from the cd
	paranoiaRead(drive, firstSector, lastSector, encoder, url.fileName(), size);

	// send an empty QByteArray to signal end of data.
	data(QByteArray());

	cdda_close(drive);

	finished();
}

void AudioCDProtocol::stat(const KURL & url)
{
	struct cdrom_drive * drive = initRequest(url);
	if (!drive)
		return;

	bool isFile = !d->fname.isEmpty();

	// the track number. 0 if ripping
	// the whole CD.
	uint trackNumber = d->req_track + 1;

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

	const mode_t _umask = ::umask(0);
	::umask(_umask);

	atom.m_uds = KIO::UDS_ACCESS;
	atom.m_long = 0666 & (~_umask);
	entry.append(atom);

	atom.m_uds = KIO::UDS_SIZE;
	if (!isFile)
	{
		atom.m_long = cdda_tracks(drive);
	}
	else
	{
			AudioCDEncoder *encoder = determineEncoder(d->fname);
			long firstSector, lastSector;
			getSectorsForRequest(drive, firstSector, lastSector);
			atom.m_long = fileSize(firstSector, lastSector, encoder);
	}

	entry.append(atom);

	statEntry(entry);

	cdda_close(drive);

	finished();
}

static void app_entry(UDSEntry& e, unsigned int uds, const QString& str)
{
	UDSAtom a;
	a.m_uds = uds;
	a.m_str = str;
	e.append(a);
}

static void app_entry(UDSEntry& e, unsigned int uds, long l)
{
	UDSAtom a;
	a.m_uds = uds;
	a.m_long = l;
	e.append(a);
}

static void app_dir(UDSEntry& e, const QString & n, size_t s)
{
	e.clear();
	app_entry(e, KIO::UDS_NAME, QFile::decodeName(n.local8Bit()));
	app_entry(e, KIO::UDS_FILE_TYPE, S_IFDIR);
	app_entry(e, KIO::UDS_ACCESS, 0400);
	app_entry(e, KIO::UDS_SIZE, s);
	app_entry(e, KIO::UDS_MIME_TYPE, "inode/directory");
}

static void app_file(UDSEntry& e, const QString & n, size_t s)
{
	e.clear();
	app_entry(e, KIO::UDS_NAME, QFile::decodeName(n.local8Bit()));
	app_entry(e, KIO::UDS_FILE_TYPE, S_IFREG);
	app_entry(e, KIO::UDS_ACCESS, 0400);
	app_entry(e, KIO::UDS_SIZE, s);
}

void AudioCDProtocol::listDir(const KURL & url)
{
	struct cdrom_drive * drive = initRequest(url);

	// Some error checking before proceeding
	if (!drive)
		return;

	if (d->which_dir == Unknown){
		error(KIO::ERR_DOES_NOT_EXIST, url.path());
		cdda_close(drive);
		return;
	}

	if ( !d->fname.isEmpty() ){
		error(KIO::ERR_IS_FILE, url.path());
		cdda_close(drive);
		return;
	}

	// Generate templated names every time
	// because the template might have changed.
	generateTemplateTitles();

	UDSEntry entry;
	// If the tracks should be listed in this directory
	bool list_tracks = true;

	if (d->which_dir == Info){
		CDInfoList::iterator it;
		uint count = 1;
		for ( it = d->cddbList.begin(); it != d->cddbList.end(); ++it ){
			(*it).toString();
			if(count == 1)
				app_file(entry, QString("%1.txt").arg(i18n(CDDB_INFORMATION)), ((*it).toString().length())+1);
			else
				app_file(entry, QString("%1_%2.txt").arg(i18n(CDDB_INFORMATION)).arg(count), ((*it).toString().length())+1);
			count++;
			listEntry(entry, false);
		}
		// Error
		if( count == 1 ) {
			app_file(entry, QString("%1: %2.txt").arg(i18n(CDDB_INFORMATION)).arg(CDDB::resultToString(d->cddbResult)), ((*it).toString().length())+1);
			count++;
			listEntry(entry, false);
		}

		list_tracks = false;
	}

	if (d->which_dir == Root){
		// List virtual directories.
		app_dir(entry, d->s_fullCD, encoders.count());
		listEntry(entry, false);

		// Either >0 cddb results or cddb error file
		app_dir(entry, d->s_info, d->cddbList.count());
		listEntry(entry, false);

		// List the encoders
		AudioCDEncoder *encoder;
		for ( encoder = encoders.first(); encoder; encoder = encoders.next() ){
			// Skip the directory that is in the root (you can still go in it, just don't show it)
			if( encoder == encoderTypeWAV )
				continue;
			app_dir(entry, encoder->type(), d->tracks);
			listEntry(entry, false);
		}
	}

	// Now fill in the tracks for the current directory
	if (list_tracks && d->which_dir == FullCD) {
		// if we're listing the "full CD" subdirectory :
		if ( (d->which_dir == FullCD) ) {
			AudioCDEncoder *encoder;
			for ( encoder = encoders.first(); encoder; encoder = encoders.next() ){
				if (d->cddbResult != KCDDB::CDDB::Success)
					addEntry(d->s_fullCD, encoder, drive, -1);
				else
					addEntry(d->templateAlbumName, encoder, drive, -1);
			}
		}
	}

	if (list_tracks && d->which_dir != FullCD) {
		// listing another dir than the "FullCD" one.
		for (uint trackNumber = 1; trackNumber <= d->tracks; trackNumber++)
		{
			// Skip data tracks
			if (!d->trackIsAudio[trackNumber-1])
				continue;

			switch (d->which_dir) {
				case Root:{
					addEntry(d->templateTitles[trackNumber - 1],
						encoderTypeWAV, drive, trackNumber);
					break;
				}
				case EncoderDir:
					addEntry(d->templateTitles[trackNumber - 1],
						d->encoder_dir_type, drive, trackNumber);
					break;
				case Info:
				case Unknown:
				default:
					error(KIO::ERR_INTERNAL, url.path());
					cdda_close(drive);
					return;
			}
		}
	}

	totalSize(entry.count());
	listEntry(entry, true);
	cdda_close(drive);
	finished();
}

void AudioCDProtocol::addEntry(const QString& trackTitle, AudioCDEncoder *encoder, struct cdrom_drive * drive, int trackNo)
{
	if(!encoder || !drive)
		return;

	long theFileSize = 0;
	if (trackNo == -1)
	{ // adding entry for the full CD
		theFileSize = fileSize(cdda_track_firstsector(drive, 1),
				 cdda_track_lastsector(drive, cdda_tracks(drive)),
				 encoder);
	}
	else
	{ // adding one regular track
		long firstSector = cdda_track_firstsector(drive, trackNo);
		long lastSector = cdda_track_lastsector(drive, trackNo);
		theFileSize = fileSize(firstSector, lastSector, encoder);
	}
	UDSEntry entry;
	app_file(entry, trackTitle + QString(".")+encoder->fileType(), theFileSize);
	listEntry(entry, false);
}

long AudioCDProtocol::fileSize(long firstSector, long lastSector, AudioCDEncoder *encoder)
{
	if(!encoder)
		return 0;

	long filesize = CD_FRAMESIZE_RAW * ( lastSector - firstSector + 1 );
	long length_seconds = (filesize) / 176400;

	return encoder->size(length_seconds);
}

struct cdrom_drive *AudioCDProtocol::getDrive()
{
	QCString device(QFile::encodeName(d->device));

	struct cdrom_drive * drive = 0;

	if (!device.isEmpty() && device != "/")
		drive = cdda_identify(device, CDDA_MESSAGE_PRINTIT, 0);
	else
	{
		drive = cdda_find_a_cdrom(CDDA_MESSAGE_PRINTIT, 0);

		if (0 == drive)
		{
			if (QFile(QFile::decodeName(DEFAULT_CD_DEVICE)).exists())
				drive = cdda_identify(DEFAULT_CD_DEVICE, CDDA_MESSAGE_PRINTIT, 0);
		}
	}

	if (0 == drive) {
		kdDebug(7117) << "Can't find an audio CD on: \"" << d->device << "\"" << endl;

		QFileInfo fi(d->device);
		if(!fi.isReadable())
			error(KIO::ERR_SLAVE_DEFINED, i18n("Device doesn't have read permissions for this account.  Check the read permissions on the device."));
		else if(!fi.isWritable())
			error(KIO::ERR_SLAVE_DEFINED, i18n("Device doesn't have write permissions for this account.  Check the write permissions on the device."));
		else if(!fi.exists())
			error(KIO::ERR_DOES_NOT_EXIST, d->device);
		else error(KIO::ERR_SLAVE_DEFINED,
i18n("Unknown error.  If you have a cd in the drive try running cdparanoia -vsQ as yourself (not root). Do you see a track list? If not, make sure you have permission to access the CD device. If you are using SCSI emulation (possible if you have an IDE CD writer) then make sure you check that you have read and write permissions on the generic SCSI device, which is probably /dev/sg0, /dev/sg1, etc.. If it still does not work, try typing audiocd:/?device=/dev/sg0 (or similar) to tell kio_audiocd which device your CD-ROM is."));
		return 0;
	}

	if (0 != cdda_open(drive))
	{
		kdDebug(7117) << "cdda_open failed" << endl;
		error(KIO::ERR_CANNOT_OPEN_FOR_READING, d->device);
		cdda_close(drive);
		return 0;
	}

	return drive;
}

void AudioCDProtocol::paranoiaRead(
		struct cdrom_drive * drive,
		long firstSector,
		long lastSector,
		AudioCDEncoder* encoder,
		const QString& fileName,
		unsigned long size
	)
{
	if(!encoder || !drive)
		return;

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
			paranoiaLevel |= PARANOIA_MODE_OVERLAP;
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

	long currentSector(firstSector);

	unsigned long processed = encoder->readInit(CD_FRAMESIZE_RAW * (lastSector - firstSector + 1));
	// TODO test for errors (processed<0)?
	processedSize(processed);
	bool ok = true;

	unsigned long lastSize = size;
	unsigned long diff = 0;

	paranoia_read_limited_error = 0;
	int warned = 0;
	while (currentSector <= lastSector)
	{
		// TODO make the 5 configurable? The default in the lib is 20 fyi
		int16_t * buf = paranoia_read_limited(paranoia, paranoiaCallback, 5);
		if( warned == 0 && paranoia_read_limited_error >= 5 && d->reportErrors ){
			warning(i18n("AudioCD: Disk damage detected on this track, risk of data corruption."));
			warned = 1;
		}
		if (0 == buf) {
			kdDebug(7117) << "Unrecoverable error in paranoia_read" << endl;
			ok = false;
			error( ERR_SLAVE_DEFINED, i18n( "Error reading audio data for %1 from the CD" ).arg( fileName ) );
			break;
		}

		++currentSector;

		int encoderProcessed = encoder->read(buf, CD_FRAMESAMPLES);
		if(encoderProcessed == -1){
			kdDebug(7117) << "Encoder processing error, stopping." << endl;
			ok = false;
			QString errMsg = i18n( "Couldn't read %1: encoding failed" ).arg( fileName );
			QString details = encoder->lastErrorMessage();
			if ( !details.isEmpty() )
			    errMsg += "\n" + details;
			error( ERR_SLAVE_DEFINED, errMsg );
			break;
		}
		processed += encoderProcessed;

		/**
		 * Because compression size is so 'unknown' use some guesswork
		 *
		 * 1) First assume that the reported size is correct and
		 * only change the totalSize if the guess it outside a range of %5.
		 * 2) Only increase in size unless the decrease is %5 of last estimate.
		 * This prevents continues small changes which is just annoying.
		 */
		unsigned long end = lastSector - firstSector;
		unsigned long cur = currentSector - firstSector;
		unsigned long estSize = (processed / cur ) * end;

		// If our guess is within 5% of reported
		// size then use the reported size.
		unsigned long guess = (long)((100/(float)size)*estSize);
		if((guess > 97 && guess < 103) || estSize == 0){
			if(processed > lastSize){
				totalSize(processed+1);
				lastSize = processed;
			}
		}
		else{
			float percentDone = ((float)cur/(float)end);
			// Calculate estimated amount that will be wrong
			diff = estSize - lastSize;
			diff = (diff*(unsigned long)((100/(float)end)*(end-cur)))/2;
			// Need 1% of data calculated as initial buffer, use %2 to be safe
			if( percentDone < .02 ){
				//qDebug("val: %f, diff: %ld", ((float)cur/(float)end), diff);
				diff = 0;
			}

			// We are growing larger, increase total.
			if(lastSize < estSize){
				//qDebug("lastGuess: %ld, guess: %ld diff: %ld", lastSize, estSize, diff);
				totalSize(estSize+diff);
				lastSize = estSize+diff;
			}
			else{
				int margin = (int)((percentDone)*75);
				// Don't bother really trying until almost half way done.
				if( percentDone <= .40 )
					margin = 7;
				unsigned long low = lastSize - lastSize/margin;
				if(estSize < low){
					//qDebug("low: %ld, estSize: %ld, num: %i", low, estSize, margin);
					totalSize( estSize );
					lastSize = estSize;
				}
			}
		}
		/**
		 * End estimation.
		 */
		//qDebug("processed: %ld, totalSize: %ld", processed, estSize);
		processedSize(processed);
	}

	if(processed > size)
		totalSize(processed);

	long encoderProcessed = encoder->readCleanup();
	if ( encoderProcessed >= 0 ) {
		processed += encoderProcessed;
		if(processed > size)
			totalSize(processed);
		processedSize(processed);
	}
	else if ( ok ) // i.e. no error message already emitted
		error( ERR_SLAVE_DEFINED, i18n( "Couldn't read %1: encoding failed" ).arg( fileName ) );

	paranoia_free(paranoia);
	paranoia = 0;
}

/**
 * Read the settings from the URL
 * @see loadSettings()
 */
void AudioCDProtocol::parseURLArgs(const KURL & url)
{
	d->clearURLargs();

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
			d->device = value;
		else if (attribute == QFL1("paranoia_level"))
			d->paranoiaLevel = value.toInt();
		else if (attribute == QFL1("fileNameTemplate"))
			d->fileNameTemplate = value;
		else if (attribute == QFL1("albumNameTemplate"))
			d->albumTemplate = value;
		else if (attribute == QFL1("cddbChoice"))
			d->cddbUserChoice = value.toInt();
		else if (attribute == QFL1("niceLevel")){
			int niceLevel = value.toInt();
			if(setpriority(PRIO_PROCESS, getpid(), niceLevel) != 0)
				kdDebug(7117) << "Setting nice level to (" << niceLevel << ") failed." << endl;
		}
	}
}

/**
 * Read the settings set by the kcm modules
 * @see parseURLArgs()
 */
void AudioCDProtocol::loadSettings()
{
	KConfig *config = new KConfig(QFL1("kcmaudiocdrc"), true /*readonly*/, false /*no kdeglobals*/);

	config->setGroup(QFL1("CDDA"));

	if (!config->readBoolEntry(QFL1("autosearch"),true)) {
		d->device = config->readEntry(QFL1("device"),QFL1(DEFAULT_CD_DEVICE));
	}

	d->paranoiaLevel = 1; // enable paranoia error correction, but allow skipping

	if (config->readBoolEntry("disable_paranoia",false)) {
		d->paranoiaLevel = 0; // disable all paranoia error correction
	}

	if (config->readBoolEntry("never_skip",true)) {
		d->paranoiaLevel = 2;
		// never skip on errors of the medium, should be default for high quality
	}

	d->reportErrors = config->readBoolEntry( "report_errors", false );

	if(config->hasKey("niceLevel")) {
		int niceLevel = config->readNumEntry("niceLevel", 0);
		if(setpriority(PRIO_PROCESS, getpid(), niceLevel) != 0)
			kdDebug(7117) << "Setting nice level to (" << niceLevel << ") failed." << endl;
	}

	// The default track filename template
	config->setGroup("FileName");
	d->fileNameTemplate = config->readEntry("file_name_template", "%{albumartist} - %{number} - %{title}");
	d->albumTemplate = config->readEntry("album_template", "%{albumartist} - %{albumtitle}");
	d->rsearch = config->readEntry("regexp_search");
	d->rreplace = config->readEntry("regexp_replace");
	// if the regular expressions are enclosed in qoutes. remove them
	// otherwise it is not possible to search for a space " ", since an empty (only spaces) value is not 
	// supported by KConfig, so the space has to be qouted, but then here the regexp searches really for " "
	// instead of just the space. Alex
	QRegExp qoutedString("^\".*\"$");
	if (qoutedString.exactMatch(d->rsearch))
	{
        	d->rsearch=d->rsearch.mid(1, d->rsearch.length()-2);
	}
	if (qoutedString.exactMatch(d->rreplace))
	{
		d->rreplace=d->rreplace.mid(1, d->rreplace.length()-2);
	}

	// Tell the encoders to load their settings
	AudioCDEncoder *encoder = encoders.first();
	while ( encoder ) {
		if ( encoder->init() ) {
			kdDebug(7117) << "Encoder for " << encoder->type() << " is available." << endl;
			encoder->loadSettings();
			encoder = encoders.next();
		} else {
			kdDebug(7117) << "Encoder for " << encoder->type() << " is NOT available." << endl;
			encoders.remove( encoder );
			encoder = encoders.current();
		}
	}

	delete config;
}

/**
 * Generates the track titles from the template using the cddb information.
 */
void AudioCDProtocol::generateTemplateTitles()
{
	d->templateTitles.clear();
	if (d->cddbResult != KCDDB::CDDB::Success)
	{
		for (unsigned int i = 0; i < d->tracks; i++){
			QString n;
			d->templateTitles.append( i18n("Track %1").arg(n.sprintf("%02d", i + 1)));
		}
		return;
	}

	KCDDB::CDInfo info = d->cddbBestChoice;
	if(d->cddbUserChoice >= 0 && (((uint)d->cddbUserChoice) < d->cddbList.count()))
		info = d->cddbList[d->cddbUserChoice];

	// Then generate the templates
	d->templateTitles.clear();
	for (uint i = 0; i < d->tracks; i++) {
		QMap<QString, QString> macros;
		macros["albumartist"] = info.artist;
		macros["albumtitle"] = info.title;
		macros["title"] = (info.trackInfoList[i].title);
		QString n;
		macros["number"] = n.sprintf("%02d", i + 1);
		//macros["number"] = QString("%1").arg(i+1, 2, 10);
		macros["genre"] = info.genre;
		macros["year"] = QString::number(info.year);

		QString title = KMacroExpander::expandMacros(d->fileNameTemplate, macros, '%').replace('/', QFL1("%2F"));
		title.replace( QRegExp(d->rsearch), d->rreplace );
		d->templateTitles.append(title);
	}

	QMap<QString, QString> macros;
	macros["albumartist"] = info.artist;
	macros["albumtitle"] = info.title;
	macros["genre"] = info.genre;
	macros["year"] = QString::number(info.year);
	d->templateAlbumName = KMacroExpander::expandMacros(d->albumTemplate, macros, '%').replace('/', QFL1("%2F"));
	d->templateAlbumName.replace( QRegExp(d->rsearch), d->rreplace );
}

/**
 * Based upon the cdparinoia ripping application
 * Only output BAD stuff
 * The higher the paranoia_read_limited_error the worse the problem is
 * FYI: PARANOIA_CB_READ & PARANOIA_CB_VERIFY happen continusly when ripping
 */
void paranoiaCallback(long, int function)
{
	switch(function){
		case PARANOIA_CB_VERIFY:
			//kdDebug(7117) << "PARANOIA_CB_VERIFY" << endl;
			break;

		case PARANOIA_CB_READ:
			//kdDebug(7117) << "PARANOIA_CB_READ" << endl;
			break;

		case PARANOIA_CB_FIXUP_EDGE:
			//kdDebug(7117) << "PARANOIA_CB_FIXUP_EDGE" << endl;
			paranoia_read_limited_error = 2;
			break;

		case PARANOIA_CB_FIXUP_ATOM:
			//kdDebug(7117) << "PARANOIA_CB_FIXUP_ATOM" << endl;
			paranoia_read_limited_error = 6;
			break;

		case PARANOIA_CB_READERR:
			kdDebug(7117) << "PARANOIA_CB_READERR" << endl;
			paranoia_read_limited_error = 6;
			break;

		case PARANOIA_CB_SKIP:
			kdDebug(7117) << "PARANOIA_CB_SKIP" << endl;
			paranoia_read_limited_error = 8;
			break;

		case PARANOIA_CB_OVERLAP:
			//kdDebug(7117) << "PARANOIA_CB_OVERLAP" << endl;
			break;

		case PARANOIA_CB_SCRATCH:
			kdDebug(7117) << "PARANOIA_CB_SCRATCH" << endl;
			paranoia_read_limited_error = 7;
			break;

		case PARANOIA_CB_DRIFT:
			//kdDebug(7117) << "PARANOIA_CB_DRIFT" << endl;
			paranoia_read_limited_error = 4;
			break;

		case PARANOIA_CB_FIXUP_DROPPED:
			kdDebug(7117) << "PARANOIA_CB_FIXUP_DROPPED" << endl;
			paranoia_read_limited_error = 5;
			break;

		case PARANOIA_CB_FIXUP_DUPED:
			kdDebug(7117) << "PARANOIA_CB_FIXUP_DUPED" << endl;
			paranoia_read_limited_error = 5;
			break;
	}
}

