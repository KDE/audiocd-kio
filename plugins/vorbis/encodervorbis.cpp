/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000, 2001, 2002 Michael Matz <matz@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
  Copyright (C) 2001 Adrian Schroeter <adrian@suse.de>
  Copyright (C) 2003 Richard Lärkäng <richard@goteborg.utfors.se>
  Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
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

#include "encodervorbis.h"
#include "audiocd_vorbis_encoder.h"
#include "encodervorbisconfig.h"

#ifdef HAVE_VORBIS

#include <vorbis/vorbisenc.h>
#include <time.h>
#include <stdlib.h>
#include <kconfig.h>
#include <knuminput.h>
#include <qgroupbox.h>

#include <kglobal.h>  
#include <klocale.h>

extern "C"
{
  KDE_EXPORT void create_audiocd_encoders(KIO::SlaveBase *slave, QPtrList<AudioCDEncoder> &encoders)
  {
    encoders.append(new EncoderVorbis(slave));
  }
}

// these are the approx. bitrates for the current 5 Vorbis modes
static int vorbis_nominal_bitrates[] = { 128, 160, 192, 256, 350 };
static int vorbis_bitrates[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 350 };

class EncoderVorbis::Private {

public:
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
};

EncoderVorbis::EncoderVorbis(KIO::SlaveBase *slave) : AudioCDEncoder(slave) {
  d = new Private();
}

EncoderVorbis::~EncoderVorbis(){
  delete d;
}

QWidget* EncoderVorbis::getConfigureWidget(KConfigSkeleton** manager) const {
  (*manager) = Settings::self();
  KGlobal::locale()->insertCatalogue("audiocd_encoder_vorbis");
  EncoderVorbisConfig *config = new EncoderVorbisConfig();
  config->kcfg_vorbis_quality->setRange(0.0, 10.0, 0.2, true);
  config->vorbis_bitrate_settings->hide();
  return config;
} 

bool EncoderVorbis::init(){
  vorbis_info_init(&d->vi);
  vorbis_comment_init(&d->vc);

  vorbis_comment_add_tag
    (
     &d->vc,
     const_cast<char *>("kde-encoder"),
     const_cast<char *>("kio_audiocd")
    );
  return true;
}

void EncoderVorbis::loadSettings(){
  Settings *settings = Settings::self();
  
  d->vorbis_encode_method = settings->vorbis_enc_method();
  d->vorbis_quality = settings->vorbis_quality();

  if ( settings->set_vorbis_min_br()) {
    d->vorbis_bitrate_lower = vorbis_bitrates[settings->vorbis_min_br()] * 1000;
  } else {
    d->vorbis_bitrate_lower = -1;
  }

  if ( settings->set_vorbis_max_br() ) {
    d->vorbis_bitrate_upper = vorbis_bitrates[settings->vorbis_max_br()] * 1000;
  } else {
    d->vorbis_bitrate_upper = -1;
  }

  // this is such a hack!
  if ( d->vorbis_bitrate_upper != -1 && d->vorbis_bitrate_lower != -1 ) {
    d->vorbis_bitrate = 104000; // empirically determined ...?!
  } else {
    d->vorbis_bitrate = 160 * 1000;
  }

  if ( settings->set_vorbis_nominal_br() ) {
    d->vorbis_bitrate_nominal = vorbis_nominal_bitrates[settings->vorbis_nominal_br()] * 1000;
    d->vorbis_bitrate = d->vorbis_bitrate_nominal;
  } else {
    d->vorbis_bitrate_nominal = -1;
  }

  d->write_vorbis_comments = settings->vorbis_comments();

  // Now that we have read in the settings apply them to the encoder lib
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

}

long EncoderVorbis::flush_vorbis(void) {
  long processed(0);

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

        if (d->og.header_len) {
          output.setRawData(oggheader, d->og.header_len);
          ioslave->data(output);
          output.resetRawData(oggheader, d->og.header_len);
        }

        if (d->og.body_len) {
          output.setRawData(oggbody, d->og.body_len);
          ioslave->data(output);
          output.resetRawData(oggbody, d->og.body_len);
        }
        processed +=  d->og.header_len + d->og.body_len;
      }
    }
  }
  return processed;
}

unsigned long EncoderVorbis::size(long time_secs) const {
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

const char * EncoderVorbis::mimeType() const{
  return "audio/vorbis";
}

long EncoderVorbis::readInit(long /*size*/){
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

    if (d->og.header_len) {
      output.setRawData(oggheader, d->og.header_len);
      ioslave->data(output);
      output.resetRawData(oggheader, d->og.header_len);
    }

    if (d->og.body_len) {
      output.setRawData(oggbody, d->og.body_len);
      ioslave->data(output);
      output.resetRawData(oggbody, d->og.body_len);
    }
  }
  return 0;
}

long EncoderVorbis::read(int16_t * buf, int frames){
  int i;
  float **buffer=vorbis_analysis_buffer(&d->vd,frames);

  /* uninterleave samples */
  for(i=0;i<frames;i++){
    buffer[0][i]=buf[2*i]/32768.0;
    buffer[1][i]=buf[2*i+1]/32768.0;
  }

  /* process chunk of data */
  vorbis_analysis_wrote(&d->vd,i);
  return flush_vorbis();
}

long EncoderVorbis::readCleanup(){
  // send end-of-stream and flush the encoder
  vorbis_analysis_wrote(&d->vd,0);
  long processed = flush_vorbis();
  ogg_stream_clear(&d->os);
  vorbis_block_clear(&d->vb);
  vorbis_dsp_clear(&d->vd);
  vorbis_info_clear(&d->vi);
  return processed;
}

void EncoderVorbis::fillSongInfo( KCDDB::CDInfo info, int track, const QString &comment )
{
  if( !d->write_vorbis_comments )
    return;

  typedef QPair<QCString, QVariant> CommentField;
  QValueList<CommentField> commentFields;

  commentFields.append(CommentField("title", info.trackInfoList[track].get("title")));
  commentFields.append(CommentField("artist", info.get("artist")));
  commentFields.append(CommentField("album", info.get("title")));
  commentFields.append(CommentField("genre", info.get("genre")));
  commentFields.append(CommentField("tracknumber", QString::number(track+1)));
  commentFields.append(CommentField("comment", comment));
	
  if (info.get("year").toInt() > 0) {
    QDateTime dt(QDate(info.get("year").toInt(), 1, 1));
    commentFields.append(CommentField("date", dt.toString(Qt::ISODate).utf8().data()));
  }

  for(QValueListIterator<CommentField> it = commentFields.begin(); it != commentFields.end(); ++it) {

    // if the value is not empty
    if(!(*it).second.toString().isEmpty()) {

      char *key = qstrdup((*it).first);
      char *value = qstrdup((*it).second.toString().utf8().data());

      vorbis_comment_add_tag(&d->vc, key, value);

      delete [] key;
      delete [] value;
    }
  }
}

#endif // HAVE_VORBIS

