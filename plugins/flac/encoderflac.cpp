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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "encoderflac.h"

#ifdef HAVE_LIBFLAC

#include <FLAC/format.h>
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>

#include <kconfig.h>
#include <kdebug.h>


extern "C"
{
  KDE_EXPORT void create_audiocd_encoders(KIO::SlaveBase *slave, QPtrList<AudioCDEncoder> &encoders)
  {
    encoders.append(new EncoderFLAC(slave));
  }
}

class EncoderFLAC::Private {

public:
    FLAC__StreamEncoder *encoder;
    FLAC__StreamMetadata** metadata;
    KIO::SlaveBase* ioslave;
    unsigned long data;
};

static FLAC__StreamEncoderWriteStatus WriteCallback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
    EncoderFLAC::Private *d = (EncoderFLAC::Private*)client_data;

    d->data += bytes;

    QByteArray output;

    if (bytes) {
       output.setRawData((const char*)buffer, bytes);
       d->ioslave->data(output);
       output.resetRawData((const char*)buffer, bytes);
    }

    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static void MetadataCallback (const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	// We do not have seekable writeback so we just discard the updated metadata

}

/*
static FLAC__SeekableStreamEncoderSeekStatus  SeekCallback(const FLAC__SeekableStreamEncoder *encoder,  FLAC__uint64 absolute_byte_offset, void *client_data)
{} ; */




EncoderFLAC::EncoderFLAC(KIO::SlaveBase *slave) : AudioCDEncoder(slave) {
    d = new Private();
    d->ioslave = slave;
    d->encoder = 0;
}

EncoderFLAC::~EncoderFLAC() {
    if (d->encoder) FLAC__stream_encoder_delete(d->encoder);
    delete d;
}

bool EncoderFLAC::init(){
    d->encoder = FLAC__stream_encoder_new();
    d->metadata = 0;
    d->data = 0;
    return true;
}

void EncoderFLAC::loadSettings() {
//  config->setGroup("FLAC");

}

// Estimate size to be 5/8 of uncompresed size.
unsigned long EncoderFLAC::size(long time_secs) const {
   long uncompressed = (time_secs * (44100*2*2));
   return (uncompressed/8)*5 + 1000;
}

long EncoderFLAC::readInit(long size) {
    kdDebug(7117) << "EncoderFLAC::readInit() called"<< endl;
    d->data = 0;
    FLAC__stream_encoder_set_write_callback(d->encoder, WriteCallback);
    FLAC__stream_encoder_set_metadata_callback(d->encoder, MetadataCallback);
    FLAC__stream_encoder_set_client_data(d->encoder, d);

    // The options match approximely those of flac compression-level-3
    FLAC__stream_encoder_set_do_mid_side_stereo(d->encoder, true);
    FLAC__stream_encoder_set_loose_mid_side_stereo(d->encoder, true); // flac -M
    FLAC__stream_encoder_set_max_lpc_order(d->encoder, 6);            // flac -l6
    FLAC__stream_encoder_set_min_residual_partition_order(d->encoder, 3);
    FLAC__stream_encoder_set_max_residual_partition_order(d->encoder, 3); // flac -r3,3
    FLAC__stream_encoder_set_blocksize(d->encoder, 4608);
    FLAC__stream_encoder_set_streamable_subset(d->encoder, true);
    if (size > 0)
        FLAC__stream_encoder_set_total_samples_estimate(d->encoder, size/4);

    FLAC__stream_encoder_init(d->encoder);
    return d->data;
}

long EncoderFLAC::read(int16_t * buf, int frames)
{
    unsigned long olddata = d->data;
    FLAC__int32 *buffer = new FLAC__int32[frames*2];
    for(int i=0; i<frames*2;i++) {
       buffer[i] = (FLAC__int32)buf[i];
    }

    bool res = FLAC__stream_encoder_process_interleaved (d->encoder, buffer, frames);
    delete[] buffer;
    return d->data - olddata;
}

long EncoderFLAC::readCleanup()
{
    FLAC__stream_encoder_finish(d->encoder);
//    FLAC__stream_encoder_delete(d->encoder);
    if (d->metadata) {
	 FLAC__metadata_object_delete(d->metadata[0]);
         delete[] d->metadata;
	 d->metadata = 0;
    }
    return 0;
}

void EncoderFLAC::fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment )
{
    d->metadata = new FLAC__StreamMetadata*[1];
    d->metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
//    d->metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
//    d->metadata[2] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE)

    typedef QPair<QString, QVariant> Comment;
    Comment comments[7] = { Comment("Title", info.trackInfoList[track].get("title")),
	    	    	    Comment("Artist", info.get("artist")),
	    	    	    Comment("Album",  info.get("title")),
	    	    	    Comment("Genre",  info.get("genre")),
	    	    	    Comment("Tracknumber", QString::number(track+1)),
	                    Comment("Comment", comment),
	    	    	    Comment("Date", QString::null )};
    if (info.get("Year").toInt() > 0) {
    	QDateTime dt(QDate(info.get("Year").toInt(), 1, 1));
    	comments[6] = Comment("Date", dt.toString(Qt::ISODate));
    }

    FLAC__StreamMetadata_VorbisComment_Entry entry;
    QString field;
    QCString cfield;
    int num_comments = 0;

    for(int i=0; i<7; i++) {
	if (!comments[i].second.toString().isEmpty()) {
	    field = comments[i].first+"="+comments[i].second.toString();
	    cfield = field.utf8();
    	    entry.entry = (FLAC__byte*)qstrdup(cfield);
	    entry.length = cfield.length();
	    // Insert in vorbiscomment and assign ownership of pointers to FLAC
    	    FLAC__metadata_object_vorbiscomment_insert_comment(d->metadata[0], num_comments, entry, false);
	    num_comments++;
	}
    }

    FLAC__stream_encoder_set_metadata(d->encoder, d->metadata, 1);
}

#endif // HAVE_LIBFLAC

