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

#include "encoderflac.h"

#include <FLAC/format.h>
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>

#if defined(FLAC_API_VERSION_CURRENT) && FLAC_API_VERSION_CURRENT > 7
#include "audiocd_flac_encoder.h"
#endif

#include <KConfig>
#include "audiocd_kio_debug.h"

#include <QDateTime>
#include <QPair>

Q_LOGGING_CATEGORY(AUDIOCD_KIO_LOG, "kf.kio.slaves.audiocd")

extern "C"
{
  AUDIOCDPLUGINS_EXPORT void create_audiocd_encoders(KIO::SlaveBase *slave, QList<AudioCDEncoder*> &encoders)
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
#if defined(FLAC_API_VERSION_CURRENT) && FLAC_API_VERSION_CURRENT > 7
    unsigned compression_level;
#endif
};

static FLAC__StreamEncoderWriteStatus WriteCallback(const FLAC__StreamEncoder *encoder,
                                                    const FLAC__byte buffer[],
#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
                                                    unsigned bytes,
#else
                                                    size_t bytes,
#endif
                                                    unsigned samples,
                                                    unsigned current_frame,
                                                    void *client_data)
{
    Q_UNUSED(encoder)
    Q_UNUSED(samples)
    Q_UNUSED(current_frame)

    EncoderFLAC::Private *d = (EncoderFLAC::Private*)client_data;

    d->data += bytes;

    QByteArray output;

    if (bytes) {
       output = QByteArray::fromRawData((const char*)buffer, bytes);
       d->ioslave->data(output);
       output.clear();
    }

    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static void MetadataCallback (const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	// We do not have seekable writeback so we just discard the updated metadata
    Q_UNUSED(encoder)
    Q_UNUSED(metadata)
    Q_UNUSED(client_data)
}

/*
static FLAC__SeekableStreamEncoderSeekStatus  SeekCallback(const FLAC__SeekableStreamEncoder *encoder,  FLAC__uint64 absolute_byte_offset, void *client_data)
{} ; */


EncoderFLAC::EncoderFLAC(KIO::SlaveBase *slave) : AudioCDEncoder(slave) {
    d = new Private();
    d->ioslave = slave;
    d->encoder = nullptr;
#if defined(FLAC_API_VERSION_CURRENT) && FLAC_API_VERSION_CURRENT > 7
    d->compression_level = 5;
#endif
}

EncoderFLAC::~EncoderFLAC() {
    if (d->encoder) FLAC__stream_encoder_delete(d->encoder);
    delete d;
}

QWidget* EncoderFLAC::getConfigureWidget(KConfigSkeleton** manager) const {
#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
    Q_UNUSED(manager);
    return NULL;
#else
    (*manager) = Settings::self();
    return new EncoderFLACConfig();
#endif
}

bool EncoderFLAC::init(){
    d->encoder = FLAC__stream_encoder_new();
    d->metadata = nullptr;
    d->data = 0;
    return true;
}

void EncoderFLAC::loadSettings() {
#if defined(FLAC_API_VERSION_CURRENT) && FLAC_API_VERSION_CURRENT > 7
    Settings *settings = Settings::self();
    d->compression_level = settings->flac_compression_level();
    if (d->compression_level > 8) d->compression_level = 5;
#endif
}

// Estimate size to be 5/8 of uncompressed size.
unsigned long EncoderFLAC::size(long time_secs) const {
   long uncompressed = (time_secs * (44100*2*2));
   return (uncompressed/8)*5 + 1000;
}

long EncoderFLAC::readInit(long size) {
    qCDebug(AUDIOCD_KIO_LOG) << "EncoderFLAC::readInit() called";
    d->data = 0;
#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
    FLAC__stream_encoder_set_write_callback(d->encoder, WriteCallback);
    FLAC__stream_encoder_set_metadata_callback(d->encoder, MetadataCallback);
    FLAC__stream_encoder_set_client_data(d->encoder, d);
#endif

#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
    // The options match approximately those of flac compression-level-5
    FLAC__stream_encoder_set_do_mid_side_stereo(d->encoder, true);
    FLAC__stream_encoder_set_max_lpc_order(d->encoder, 8);            // flac -l8
    FLAC__stream_encoder_set_min_residual_partition_order(d->encoder, 3);
    FLAC__stream_encoder_set_max_residual_partition_order(d->encoder, 3); // flac -r3,3
#else
    FLAC__stream_encoder_set_compression_level(d->encoder, d->compression_level);
#endif

    FLAC__stream_encoder_set_streamable_subset(d->encoder, true);
    if (size > 0)
        FLAC__stream_encoder_set_total_samples_estimate(d->encoder, size/4);

#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
    FLAC__stream_encoder_init(d->encoder);
#else
    FLAC__stream_encoder_init_stream(d->encoder, WriteCallback, nullptr, nullptr, MetadataCallback, d);
#endif
    return d->data;
}

long EncoderFLAC::read(qint16 * buf, int frames)
{
    unsigned long olddata = d->data;
    FLAC__int32 *buffer = new FLAC__int32[frames*2];
    for(int i=0; i<frames*2;i++) {
       buffer[i] = (FLAC__int32)buf[i];
    }

    FLAC__stream_encoder_process_interleaved(d->encoder, buffer, frames);
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
	 d->metadata = nullptr;
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
    Comment comments[7] = { Comment(QLatin1String( "TITLE" ), info.track(track-1).get(Title)),
				Comment(QLatin1String( "ARTIST" ), info.track(track-1).get(Artist)),
				Comment(QLatin1String( "ALBUM" ),  info.get(Title)),
				Comment(QLatin1String( "GENRE" ),  info.get(Genre)),
				Comment(QLatin1String( "TRACKNUMBER" ), QString::number(track)),
				Comment(QLatin1String( "COMMENT" ), comment),
				Comment(QLatin1String( "DATE" ), QVariant(QString()) )};
    if (info.get(Year).toInt() > 0) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	const QDateTime dt = QDate(info.get(Year).toInt(), 1, 1).startOfDay();
#else
	QDateTime dt(QDate(info.get(Year).toInt(), 1, 1));
#endif
	comments[6] = Comment(QLatin1String( "DATE" ), dt.toString(Qt::ISODate));
    }

    FLAC__StreamMetadata_VorbisComment_Entry entry;
    QString field;
    int num_comments = 0;

    for(int i=0; i<7; i++) {
	if (!comments[i].second.toString().isEmpty()) {
	    field = comments[i].first+QLatin1Char( '=' )+comments[i].second.toString();
	    const QByteArray cfield = field.toUtf8();
	    entry.entry = (FLAC__byte*)qstrdup(cfield.data());
	    entry.length = cfield.length();
	    // Insert in vorbiscomment and assign ownership of pointers to FLAC
	    FLAC__metadata_object_vorbiscomment_insert_comment(d->metadata[0], num_comments, entry, false);
	    num_comments++;
	}
    }

    FLAC__stream_encoder_set_metadata(d->encoder, d->metadata, 1);
}
