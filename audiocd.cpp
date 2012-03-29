/*
 * Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
 * Copyright (C) 2000-2002 Michael Matz <matz@kde.org>
 * Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
 * Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
 * Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>
 * Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2004-2005 Benjamin Meyer <ben at meyerhome dot net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config-audiocd.h>

#include <kdemacros.h>
extern "C"
{
	//cdda_interface.h in cdparanoia 10.2 has a member called 'private' which the C++ compiler doesn't like
	#define private _private
	#include <cdda_interface.h>
	#include <cdda_paranoia.h>
	#undef private
	void paranoiaCallback(long, int);

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
#include <QFile>
#include <qfileinfo.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>
#include <QRegExp>
#include <QHash>
// CDDB
#include <libkcddb/client.h>
#include <libkcompactdisc/kcompactdisc.h>

using namespace KIO;

#define CDDB_INFORMATION I18N_NOOP("CDDB Information")

using namespace AudioCD;

int kdemain(int argc, char ** argv)
{
	// KApplication uses libkcddb which needs a valid kapp pointer
	// GUIenabled must be true as libkcddb sometimes wants to communicate
	// with the user
	putenv(strdup("SESSION_MANAGER="));
	//KApplication::disableAutoDcopRegistration();
	KCmdLineArgs::init(argc, argv, "kio_audiocd", 0, KLocalizedString(), 0, KLocalizedString());

	KCmdLineOptions options;
	options.add("+protocol", ki18n("Protocol name"));
	options.add("+pool", ki18n("Socket name"));
	options.add("+app", ki18n("Socket name"));
	KCmdLineArgs::addCmdLineOptions(options);
	KApplication app(true);

	kDebug(7117) << "Starting " << getpid();

	KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
	AudioCDProtocol slave(args->arg(0).toLocal8Bit(), args->arg(1).toLocal8Bit(), args->arg(2).toLocal8Bit());
	args->clear();
	slave.dispatchLoop();

	kDebug(7117) << "Done";
	return 0;
}

enum Which_dir {
	Unknown = 0, // Error
	Info, // CDDB info
	Root, // The root directory, shows all these :)
	FullCD, // Show a single file containing all of the data
	EncoderDir, // The root directory created by an encoder
	SubDir // A directory created from the Album name configuration
};

class AudioCDProtocol::Private {
public:
	Private() : s_info(i18n("Information")), s_fullCD(i18n("Full CD"))
	{
		clearURLargs();
	}

	void clearURLargs() {
		req_allTracks = false;
		which_dir = Unknown;
		req_track = -1;
		cddbUserChoice = -1;
	}

	bool tocsAreDifferent(struct cdrom_drive *drive)
	{
		if (tracks != (uint)drive->tracks) return true;
		for (int i = 0; i < drive->tracks; ++i)
		{
			if (disc_toc[i].dwStartSector != drive->disc_toc[i].dwStartSector ||
			    disc_toc[i].bFlags != drive->disc_toc[i].bFlags ||
			    disc_toc[i].bTrack != drive->disc_toc[i].bTrack) return true;
		}
		return false;
	}

	void setToc(struct cdrom_drive *drive)
	{
		for (int i = 0; i < drive->tracks; ++i)
		{
			disc_toc[i].dwStartSector = drive->disc_toc[i].dwStartSector;
			disc_toc[i].bFlags = drive->disc_toc[i].bFlags;
			disc_toc[i].bTrack = drive->disc_toc[i].bTrack;
		}
	}

	// The type/which of request
	bool req_allTracks;
	Which_dir which_dir;
	int req_track;
	QString fname;
	QString child_dir;
	AudioCDEncoder *encoder_dir_type;

	// Misc settings
	QString device; // URL settable
	int paranoiaLevel; // URL settable
	bool reportErrors;

	// Directory strings, never change after init
	const QString s_info;
	const QString s_fullCD;

	// Current CD
	TOC disc_toc[MAXTRK];
	unsigned tracks;
	bool trackIsAudio[100];

	// CDDB items
	KCDDB::Result cddbResult;
	CDInfoList cddbList;
	int cddbUserChoice; // URL settable
	KCDDB::CDInfo cddbBestChoice;

	// Template for ..
	QString fileNameTemplate; // URL settable
	QString albumNameTemplate; // URL settable
	QString fileLocationTemplate;  // URL settable
	QString rsearch;
	QString rreplace;

	// Current strings for this CD and or cddb selection
	QStringList templateTitles;
	QString templateAlbumName;
	QString templateFileLocation;
};

int paranoia_read_limited_error;

AudioCDProtocol::AudioCDProtocol(const QByteArray & protocol, const QByteArray & pool, const QByteArray & app)
	: SlaveBase(protocol, pool, app)
{
	d = new Private;

	// Add encoders
	AudioCDEncoder::findAllPlugins(this, encoders);
	encoderTypeCDA = encoderFromExtension(QLatin1String( ".cda" ));
	encoderTypeWAV = encoderFromExtension(QLatin1String( ".wav" ));
}

AudioCDProtocol::~AudioCDProtocol()
{
    while (!encoders.isEmpty())
        delete encoders.takeFirst();
    delete d;
}

AudioCDEncoder *AudioCDProtocol::encoderFromExtension(const QString& extension)
{
	AudioCDEncoder *encoder;
	for (int i = encoders.size()-1; i >= 0; --i) {
            encoder = encoders.at(i);
	    if(QLatin1String(".")+QLatin1String( encoder->fileType() ) == extension)
	        return encoder;
    	}
	Q_ASSERT(false);
	return NULL;
}

AudioCDEncoder *AudioCDProtocol::determineEncoder(const QString & filename)
{
	int len = filename.length();
	int pos = filename.lastIndexOf( QLatin1Char( '.' ));
	return encoderFromExtension(filename.right(len - pos));
}

static void setDeviceToCd(KCompactDisc *cd, struct cdrom_drive *drive)
{
#if defined(HAVE_CDDA_IOCTL_DEVICE)
	cd->setDevice(QLatin1String( drive->ioctl_device_name ), 50, false);
#elif defined(__FreeBSD__) || defined(__DragonFly__)
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
		kWarning(7117) << "Found an ATAPI device, assuming it is the one specified by the user.";
		cd->setDevice( drive->cdda_device_name );
	}
	else
	{
		kDebug(7117) << "Found a SCSI or ATAPICAM device.";
		if ( strlen(drive->dev->device_path) > 0 )
		{
			cd->setDevice( drive->dev->device_path );
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
			kDebug(7117) << "  Using derived name " << devname;
			cd->setDevice( devname );
		}
	}
#else
#ifdef __GNUC__
	#warning audiocd ioslave is not going to work for you
#endif
#endif
}

struct cdrom_drive * AudioCDProtocol::initRequest(const KUrl & url)
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

	if (d->tocsAreDifferent(drive))
	{
		// Update our knowledge of the disc
		KCompactDisc cd(KCompactDisc::Asynchronous);
		setDeviceToCd(&cd, drive);
		d->setToc(drive);

		d->tracks = cd.tracks();
		for(uint i=0; i< cd.tracks(); i++)
			d->trackIsAudio[i] = cd.isAudio(i+1);

		KCDDB::Client c;
		d->cddbResult = c.lookup(cd.discSignature());
		if (d->cddbResult == Success)
		{
			d->cddbList = c.lookupResponse();
			d->cddbBestChoice = d->cddbList.first();
		}
		generateTemplateTitles();
	}

	// Determine what file or folder that is wanted.
	QString path = url.path();
	if (!path.isEmpty() && path[0] == QLatin1Char( '/' ))
		path = path.mid(1);

	d->req_allTracks = false;

	// See which file and directory they want
	QString remainingDirPath;
	d->which_dir = Unknown;
	if (path.isEmpty()) {
		d->which_dir = Root;
		d->encoder_dir_type = encoderTypeWAV;
		remainingDirPath = d->templateFileLocation;
		d->fname = QString();
	} else {
		for (int i = encoders.size()-1; i >= 0; --i) {
			AudioCDEncoder *encoder = encoders.at(i);
			QString encoderFileLocation = encoder->type();
		        if (!d->templateFileLocation.isEmpty())	encoderFileLocation = encoderFileLocation + QLatin1String( "/" ) + d->templateFileLocation;
			if (path == encoder->type()) {
				d->which_dir = EncoderDir;
				d->encoder_dir_type = encoder;
				remainingDirPath = encoderFileLocation.mid(path.length());
				d->fname = QString();
				break;
			} else if (encoderFileLocation.startsWith(path)) {
				d->which_dir = SubDir;
				d->encoder_dir_type = encoder;
				remainingDirPath = encoderFileLocation.mid(path.length());
				d->fname = QString();
				break;
			} else if (path.startsWith(encoderFileLocation)) {
				d->which_dir = SubDir;
				d->encoder_dir_type = encoder;
				remainingDirPath = QString();
				d->fname = path.mid(encoderFileLocation.length() + 1);
				break;
			} else if (path.startsWith(encoder->type())) {
				d->which_dir = EncoderDir;
				d->encoder_dir_type = encoder;
				remainingDirPath = QString();
				d->fname = path.mid(encoder->type().length() + 1);
			}
		}
		if ( Unknown == d->which_dir ) {
			if (path.startsWith(d->s_info)) {
				d->which_dir = Info;
				d->fname = path.mid(d->s_info.length() + 1);
			} else if (path.startsWith(d->s_fullCD)) {
				d->which_dir = FullCD;
				d->fname = path.mid(d->s_fullCD.length() + 1);
				d->req_allTracks = true;
			} else if (d->templateFileLocation.startsWith(path)) {
				d->which_dir = SubDir;
				d->encoder_dir_type = encoderTypeWAV;
				remainingDirPath = d->templateFileLocation.mid(path.length());
				d->fname = QString();
			} else if (path.startsWith(d->templateFileLocation)) {
				d->encoder_dir_type = encoderTypeWAV;
				remainingDirPath = QString();
				d->fname = path.mid(d->templateFileLocation.length() + 1);
			} else  {
				d->encoder_dir_type = encoderTypeWAV;
				remainingDirPath = QString();
				d->fname = path;
			}
		}
	}
	if (!remainingDirPath.isEmpty() && remainingDirPath[0] == QLatin1Char( '/' ))
		remainingDirPath = remainingDirPath.mid(1);
	d->child_dir = remainingDirPath.split(QLatin1String( "/" )).first();

	// See if the url is a track
	d->req_track = -1;
	if (!d->fname.isEmpty()){
		QString name(d->fname);

		// Remove extension
		int dot = name.lastIndexOf( QLatin1Char( '.' ));
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
			int start = 0;
			int end = 0;
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

	kDebug(7117) << "path=" << path << " file=" << d->fname
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

void AudioCDProtocol::get(const KUrl & url)
{
	struct cdrom_drive * drive = initRequest(url);
	if (!drive)
		return;

	if( d->fname.contains(i18n(CDDB_INFORMATION))){
		uint choice = 1;
		if(d->fname != QString::fromLatin1("%1.txt").arg(i18n(CDDB_INFORMATION))){
			choice= d->fname.section(QLatin1Char( '_' ),1,1).section(QLatin1Char( '.' ),0,0).toInt();
		}
		uint count = 1;
		CDInfoList::iterator it;
		bool found = false;
		for ( it = d->cddbList.begin(); it != d->cddbList.end(); ++it ){
			if(count == choice){
				mimeType(QLatin1String( "text/html" ));
				data(QByteArray( (*it).toString().toLatin1() ));
				// send an empty QByteArray to signal end of data.
				data(QByteArray());
				finished();
				found = true;
				break;
			}
			count++;
		}
		if(!found && d->fname.contains(i18n(CDDB_INFORMATION)+QLatin1Char( ':' ))){
			mimeType(QLatin1String( "text/html" ));
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
	if(d->cddbResult == KCDDB::Success){
		info = d->cddbBestChoice;

		int track = d->req_track+1;

		// hack
		// do we rip the whole CD?
		if (d->req_allTracks){
			track = 1;
			// YES => the title of the file is the title of the CD
			info.track(track-1).set(Title, info.get(Title));
		}
		encoder->fillSongInfo(info, track, QString());
	}
	long totalByteCount = CD_FRAMESIZE_RAW * (lastSector - firstSector + 1);
	long time_secs = (8 * totalByteCount) / (44100 * 2 * 16);

	unsigned long size = encoder->size(time_secs);
	totalSize(size);
	emit mimeType(QLatin1String(encoder->mimeType()));

	// Read data (track/disk) from the cd
	paranoiaRead(drive, firstSector, lastSector, encoder, url.fileName(), size);

	// send an empty QByteArray to signal end of data.
	data(QByteArray());

	cdda_close(drive);

	finished();
}

void AudioCDProtocol::stat(const KUrl & url)
{
	struct cdrom_drive * drive = initRequest(url);
	if (!drive)
		return;

	const bool isFile = !d->fname.isEmpty();

	// the track number. 0 if ripping
	// the whole CD.
	const uint trackNumber = d->req_track + 1;

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

	entry.insert( KIO::UDSEntry::UDS_NAME, url.fileName().replace(QLatin1Char( '/' ), QLatin1String("%2F") ));

	entry.insert( KIO::UDSEntry::UDS_FILE_TYPE,isFile ? S_IFREG : S_IFDIR);


	const mode_t _umask = ::umask(0);
	::umask(_umask);

	entry.insert(KIO::UDSEntry::UDS_ACCESS, (0666 & (~_umask)));

	if (!isFile)
	{
		entry.insert( KIO::UDSEntry::UDS_SIZE, cdda_tracks(drive));
	}
	else
	{
			AudioCDEncoder *encoder = determineEncoder(d->fname);
			long firstSector, lastSector;
			getSectorsForRequest(drive, firstSector, lastSector);
			entry.insert( KIO::UDSEntry::UDS_SIZE,fileSize(firstSector, lastSector, encoder));
	}


	statEntry(entry);

	cdda_close(drive);

	finished();
}

static void app_dir(UDSEntry& e, const QString & n, size_t s)
{
	e.clear();
	e.insert( KIO::UDSEntry::UDS_NAME, QFile::decodeName(n.toLocal8Bit()));
	e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
	e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
	e.insert( KIO::UDSEntry::UDS_SIZE, s);
	e.insert( KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String("inode/directory"));
}

static void app_file(UDSEntry& e, const QString & n, size_t s, const QString &mimetype = QString())
{
	e.clear();
	e.insert( KIO::UDSEntry::UDS_NAME, QFile::decodeName(n.toLocal8Bit()));
	e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
	e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
	e.insert( KIO::UDSEntry::UDS_SIZE, s);
	if (!mimetype.isEmpty())
		e.insert( KIO::UDSEntry::UDS_MIME_TYPE, mimetype);
}

void AudioCDProtocol::listDir(const KUrl & url)
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

	if (d->which_dir == Info){
		CDInfoList::iterator it;
		uint count = 1;
		for ( it = d->cddbList.begin(); it != d->cddbList.end(); ++it ){
			(*it).toString();
			if(count == 1)
                            app_file(entry, QString::fromLatin1("%1.txt").arg(i18n(CDDB_INFORMATION)), ((*it).toString().length())+1);
			else
                            app_file(entry, QString::fromLatin1("%1_%2.txt").arg(i18n(CDDB_INFORMATION)).arg(count), ((*it).toString().length())+1);
			count++;
			listEntry(entry, false);
		}
		// Error
		if( count == 1 ) {
                    app_file(entry, QString::fromLatin1("%1: %2.txt").arg(i18n(CDDB_INFORMATION)).arg(KCDDB::resultToString(d->cddbResult)), 0);
			count++;
			listEntry(entry, false);
		}
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
		for (int i = encoders.size()-1; i >= 0; --i) {
            		encoder = encoders.at(i);
			// Skip the directory that is in the root (you can still go in it, just don't show it)
			if( encoder == encoderTypeWAV )
				continue;
			app_dir(entry, encoder->type(), d->tracks);
			listEntry(entry, false);
		}
	}

	// Now fill in the tracks for the current directory
	if (d->which_dir == FullCD) {
		AudioCDEncoder *encoder;
		for (int i = encoders.size()-1; i >= 0; --i) {
			encoder = encoders.at(i);
			if (d->cddbResult != KCDDB::Success)
				addEntry(d->s_fullCD, encoder, drive, -1);
			else
				addEntry(d->templateAlbumName, encoder, drive, -1);
		}
	}

	if (d->which_dir == SubDir || d->which_dir == Root || d->which_dir == EncoderDir) {
		if (d->child_dir.isEmpty() || d->which_dir == Root || d->which_dir == EncoderDir)
		{
			// we are at the end of the hierarchy, list the tracks
			for (uint trackNumber = 1; trackNumber <= d->tracks; trackNumber++)
			{
				// Skip data tracks
				if (!d->trackIsAudio[trackNumber-1])
					continue;

				switch (d->which_dir) {
					case EncoderDir:
					case SubDir:
					case Root:
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

		if (!d->child_dir.isEmpty())
		{
			app_dir(entry, d->child_dir, 1);
			listEntry(entry, false);
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
	app_file(entry, trackTitle + QLatin1String(".")+QLatin1String( encoder->fileType() ), theFileSize, QLatin1String( encoder->mimeType() ));
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
	const QByteArray device(QFile::encodeName(d->device));

	struct cdrom_drive * drive = 0;

	if (!device.isEmpty() && device != "/" )
		drive = cdda_identify(device, CDDA_MESSAGE_PRINTIT, 0);
	else
	{
		drive = cdda_find_a_cdrom(CDDA_MESSAGE_PRINTIT, 0);

		if (0 == drive)
		{
			if (QFile(QFile::decodeName(KCompactDisc::defaultCdromDeviceUrl().url().toAscii())).exists())
				drive = cdda_identify(KCompactDisc::defaultCdromDeviceUrl().url().toAscii(), CDDA_MESSAGE_PRINTIT, 0);
		}
	}

	if (0 == drive) {
		kDebug(7117) << "Can't find an audio CD on: \"" << d->device << "\"";

		const QFileInfo fi(d->device);
		if(!fi.isReadable())
			error(KIO::ERR_SLAVE_DEFINED, i18n("Device does not have read permissions for this account.  Check the read permissions on the device."));
		else if(!fi.isWritable())
			error(KIO::ERR_SLAVE_DEFINED, i18n("Device does not have write permissions for this account.  Check the write permissions on the device."));
		else if(!fi.exists())
			error(KIO::ERR_DOES_NOT_EXIST, d->device);
		else error(KIO::ERR_SLAVE_DEFINED,
i18n("Unknown error.  If you have a cd in the drive try running cdparanoia -vsQ as yourself (not root). Do you see a track list? If not, make sure you have permission to access the CD device. If you are using SCSI emulation (possible if you have an IDE CD writer) then make sure you check that you have read and write permissions on the generic SCSI device, which is probably /dev/sg0, /dev/sg1, etc.. If it still does not work, try typing audiocd:/?device=/dev/sg0 (or similar) to tell kio_audiocd which device your CD-ROM is."));
		return 0;
	}

	if (0 != cdda_open(drive))
	{
		kDebug(7117) << "cdda_open failed";
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
		kDebug(7117) << "paranoia_init failed";
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
			kDebug(7117) << "Unrecoverable error in paranoia_read";
			ok = false;
			error( ERR_SLAVE_DEFINED, i18n( "Error reading audio data for %1 from the CD", fileName ) );
			break;
		}

		++currentSector;

		int encoderProcessed = encoder->read(buf, CD_FRAMESAMPLES);
		if(encoderProcessed == -1){
			kDebug(7117) << "Encoder processing error, stopping.";
			ok = false;
			QString errMsg = i18n( "Could not read %1: encoding failed", fileName );
			QString details = encoder->lastErrorMessage();
			if ( !details.isEmpty() )
			    errMsg += QLatin1Char( '\n' ) + details;
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
		//qDebug("processed: %d, totalSize: %d", processed, estSize);
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
		error( ERR_SLAVE_DEFINED, i18n( "Could not read %1: encoding failed", fileName ) );

	paranoia_free(paranoia);
	paranoia = 0;
}

/**
 * Read the settings from the URL
 * @see loadSettings()
 */
void AudioCDProtocol::parseURLArgs(const KUrl & url)
{
	d->clearURLargs();

	QString query(QUrl::fromPercentEncoding(url.query().toAscii()));

	if (query.isEmpty() || query[0] != QLatin1Char( '?' ))
		return;

	query = query.mid(1); // Strip leading '?'.

	const QStringList tokens(query.split(QLatin1Char( '&' ),QString::SkipEmptyParts));

	for (QStringList::ConstIterator it(tokens.constBegin()); it != tokens.constEnd(); ++it)
	{
		const QString token(*it);

		int equalsPos = token.indexOf(QLatin1Char( '=' ));
		if (-1 == equalsPos)
			continue;

		const QString attribute(token.left(equalsPos));
		const QString value(token.mid(equalsPos + 1));

		if (attribute == QLatin1String("device"))
			d->device = value;
		else if (attribute == QLatin1String("paranoia_level"))
			d->paranoiaLevel = value.toInt();
		else if (attribute == QLatin1String("fileNameTemplate"))
			d->fileNameTemplate = value;
		else if (attribute == QLatin1String("albumNameTemplate"))
			d->albumNameTemplate = value;
		else if (attribute == QLatin1String("fileLocationTemplate"))
			d->fileLocationTemplate = value;
		else if (attribute == QLatin1String("cddbChoice"))
			d->cddbUserChoice = value.toInt();
		else if (attribute == QLatin1String("niceLevel")){
			int niceLevel = value.toInt();
			if(setpriority(PRIO_PROCESS, getpid(), niceLevel) != 0)
				kDebug(7117) << "Setting nice level to (" << niceLevel << ") failed.";
		}
	}
}

/**
 * Read the settings set by the kcm modules
 * @see parseURLArgs()
 */
void AudioCDProtocol::loadSettings()
{
	const KConfig *config = new KConfig(QLatin1String( "kcmaudiocdrc"), KConfig::NoGlobals );
        const KConfigGroup groupCDDA( config, "CDDA" );

	if (!groupCDDA.readEntry("autosearch", true)) {
		d->device = groupCDDA.readEntry("device", QString(KCompactDisc::defaultCdromDeviceUrl().url()));
	}

	d->paranoiaLevel = 1; // enable paranoia error correction, but allow skipping

	if (groupCDDA.readEntry("disable_paranoia", false)) {
		d->paranoiaLevel = 0; // disable all paranoia error correction
	}

	if (groupCDDA.readEntry("never_skip", true)) {
		d->paranoiaLevel = 2;
		// never skip on errors of the medium, should be default for high quality
	}

	d->reportErrors = groupCDDA.readEntry( "report_errors", false );

	if(groupCDDA.hasKey("niceLevel")) {
		int niceLevel = groupCDDA.readEntry("niceLevel", 0);
		if(setpriority(PRIO_PROCESS, getpid(), niceLevel) != 0)
			kDebug(7117) << "Setting nice level to (" << niceLevel << ") failed.";
	}

	// The default track filename template
	const KConfigGroup groupFileName( config, "FileName" );
	d->fileNameTemplate = groupFileName.readEntry("file_name_template", "%{trackartist} - %{number} - %{title}");
	d->albumNameTemplate = groupFileName.readEntry("album_name_template", "%{albumartist} - %{albumtitle}");
	if (groupFileName.readEntry("show_file_location", false))
		d->fileLocationTemplate = groupFileName.readEntry("file_location_template", QString());
	else
		d->fileLocationTemplate = QString();
	d->rsearch = groupFileName.readEntry("regexp_search");
	d->rreplace = groupFileName.readEntry("regexp_replace");
	// if the regular expressions are enclosed in qoutes. remove them
	// otherwise it is not possible to search for a space " ", since an empty (only spaces) value is not
	// supported by KConfig, so the space has to be qouted, but then here the regexp searches really for " "
	// instead of just the space. Alex
	QRegExp qoutedString( QLatin1String( "^\".*\"$" ));
	if (qoutedString.exactMatch(d->rsearch))
	{
		d->rsearch=d->rsearch.mid(1, d->rsearch.length()-2);
	}
	if (qoutedString.exactMatch(d->rreplace))
	{
		d->rreplace=d->rreplace.mid(1, d->rreplace.length()-2);
	}

	// Tell the encoders to load their settings
	AudioCDEncoder *encoder;
	for (int i = encoders.size()-1; i >= 0; --i) {
		encoder = encoders.at(i);
		if (encoder->init())
			encoder->loadSettings();
		else
			encoders.removeAt(i);
	}

	delete config;
}

/**
 * Generates the track titles from the template using the cddb information.
 */
void AudioCDProtocol::generateTemplateTitles()
{
	d->templateTitles.clear();
	if (d->cddbResult != KCDDB::Success)
	{
		for (unsigned int i = 0; i < d->tracks; i++){
			QString n;
			d->templateTitles.append( i18n("Track %1", n.sprintf("%02d", i + 1)));
		}
		return;
	}

	KCDDB::CDInfo info = d->cddbBestChoice;
	if(d->cddbUserChoice >= 0 && ((d->cddbUserChoice) < d->cddbList.count()))
		info = d->cddbList[d->cddbUserChoice];

	// Then generate the templates
	d->templateTitles.clear();
	for (uint i = 0; i < d->tracks; i++) {
		QHash<QString, QString> macros;
		macros[QLatin1String( "albumartist" )] = info.get(Artist).toString();
		macros[QLatin1String( "albumtitle" )] = info.get(Title).toString();
		macros[QLatin1String( "title" )] = info.track(i).get(Title).toString();
		macros[QLatin1String( "trackartist" )] = info.track(i).get(Artist).toString();
		QString n;
		macros[QLatin1String( "number" )] = n.sprintf("%02d", i + 1);
		//macros["number"] = QString("%1").arg(i+1, 2, 10);
		macros[QLatin1String( "genre" )] = info.get(Genre).toString();
		macros[QLatin1String( "year" )] = info.get(Year).toString();

		QString title = KMacroExpander::expandMacros(d->fileNameTemplate, macros, QLatin1Char( '%' )).replace(QLatin1Char( '/' ), QLatin1String("%2F"));
		title.replace( QRegExp(d->rsearch), d->rreplace );
		d->templateTitles.append(title);
	}

	QHash<QString, QString> macros;
	macros[QLatin1String( "albumartist" )] = info.get(Artist).toString();
	macros[QLatin1String( "albumtitle" )] = info.get(Title).toString();
	macros[QLatin1String( "genre" )] = info.get(Genre).toString();
	macros[QLatin1String( "year" )] = info.get(Year).toString();
	d->templateAlbumName = KMacroExpander::expandMacros(d->albumNameTemplate, macros, QLatin1Char( '%' )).replace(QLatin1Char( '/' ), QLatin1String("%2F"));
	d->templateAlbumName.replace( QRegExp(d->rsearch), d->rreplace );

	d->templateFileLocation = KMacroExpander::expandMacros(d->fileLocationTemplate, macros, QLatin1Char( '%' ));
}

/**
 * Based upon the cdparanoia ripping application
 * Only output BAD stuff
 * The higher the paranoia_read_limited_error the worse the problem is
 * FYI: PARANOIA_CB_READ & PARANOIA_CB_VERIFY happen continusly when ripping
 */
void paranoiaCallback(long, int function)
{
	switch(function){
		case PARANOIA_CB_VERIFY:
			//kDebug(7117) << "PARANOIA_CB_VERIFY";
			break;

		case PARANOIA_CB_READ:
			//kDebug(7117) << "PARANOIA_CB_READ";
			break;

		case PARANOIA_CB_FIXUP_EDGE:
			//kDebug(7117) << "PARANOIA_CB_FIXUP_EDGE";
			paranoia_read_limited_error = 2;
			break;

		case PARANOIA_CB_FIXUP_ATOM:
			//kDebug(7117) << "PARANOIA_CB_FIXUP_ATOM";
			paranoia_read_limited_error = 6;
			break;

		case PARANOIA_CB_READERR:
			kDebug(7117) << "PARANOIA_CB_READERR";
			paranoia_read_limited_error = 6;
			break;

		case PARANOIA_CB_SKIP:
			kDebug(7117) << "PARANOIA_CB_SKIP";
			paranoia_read_limited_error = 8;
			break;

		case PARANOIA_CB_OVERLAP:
			//kDebug(7117) << "PARANOIA_CB_OVERLAP";
			break;

		case PARANOIA_CB_SCRATCH:
			kDebug(7117) << "PARANOIA_CB_SCRATCH";
			paranoia_read_limited_error = 7;
			break;

		case PARANOIA_CB_DRIFT:
			//kDebug(7117) << "PARANOIA_CB_DRIFT";
			paranoia_read_limited_error = 4;
			break;

		case PARANOIA_CB_FIXUP_DROPPED:
			kDebug(7117) << "PARANOIA_CB_FIXUP_DROPPED";
			paranoia_read_limited_error = 5;
			break;

		case PARANOIA_CB_FIXUP_DUPED:
			kDebug(7117) << "PARANOIA_CB_FIXUP_DUPED";
			paranoia_read_limited_error = 5;
			break;
	}
}

