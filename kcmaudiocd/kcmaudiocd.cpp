/*
   Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>

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

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <klineedit.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <kaboutdata.h>
#include <knuminput.h>

#include "kcmaudiocd.moc"

// MPEG Layer 3 Bitrates
static int bitrates[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };

// these are the approx. bitrates for the current 5 Vorbis modes
static int vorbis_nominal_bitrates[] = { 128, 160, 192, 256, 350 };
static int vorbis_bitrates[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 350 };

KAudiocdModule::KAudiocdModule(QWidget *parent, const char *name)
  : AudiocdConfig(parent, name), configChanged(false)
{

    setButtons(Default|Apply);

    /* This should be in audiocdConfig, but I can't figure out how to
     * make Qt Designer display KDE specific widget properties. */
    vorbis_quality->setPrecision(1);
    vorbis_quality->setRange(0.0, 10.0, 0.2, true);

    config = new KConfig("kcmaudiocdrc");

    load();

    //CDDA Options
    connect(cd_autosearch_check,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(ec_enable_check,SIGNAL(clicked()),this,SLOT(slotEcEnable()));
    connect(ec_skip_check,SIGNAL(clicked()),SLOT(slotConfigChanged()));
    connect(cd_device_string,SIGNAL(textChanged(const QString &)),SLOT(slotConfigChanged()));

    //MP3 Encoding Method
    connect(enc_method,SIGNAL(activated(int)),SLOT(slotSelectMethod(int)));
    connect(stereo,SIGNAL(activated(int)),SLOT(slotConfigChanged()));
    connect(quality,SIGNAL(valueChanged(int)),SLOT(slotConfigChanged()));
    connect(crc, SIGNAL(clicked()), SLOT(slotConfigChanged()));

    //MP3 Options
    connect(copyright,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(original,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(iso,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(id3_tag,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));

    //MP3 CBR Settings
    connect(cbr_bitrate,SIGNAL(activated(int)),SLOT(slotConfigChanged()));
    connect(vbr_mean_brate,SIGNAL(activated(int)),SLOT(slotConfigChanged()));

    //MP3 VBR Groupbox
    connect(vbr_average_br,SIGNAL(clicked()),this,SLOT(slotUpdateVBRWidgets()));
    connect(vbr_min_hard,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(vbr_min_br,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(vbr_min_br,SIGNAL(clicked()),this,SLOT(slotUpdateVBRWidgets()));
    connect(vbr_min_brate,SIGNAL(activated(int)),SLOT(slotConfigChanged()));

    connect(vbr_max_br,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(vbr_max_br,SIGNAL(clicked()),this,SLOT(slotUpdateVBRWidgets()));
    connect(vbr_max_brate,SIGNAL(activated(int)),SLOT(slotConfigChanged()));

    connect(vbr_xing_tag,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));

    //MP3 Filter
    connect(enable_lowpass,SIGNAL(clicked()),this,SLOT(slotChangeFilter()));
    connect(enable_highpass,SIGNAL(clicked()),this,SLOT(slotChangeFilter()));
    connect(set_lpf_width,SIGNAL(clicked()),this,SLOT(slotChangeFilter()));
    connect(set_hpf_width,SIGNAL(clicked()),this,SLOT(slotChangeFilter()));

    connect(lowfilterwidth,SIGNAL(valueChanged(int)),SLOT(slotConfigChanged()));
    connect(highfilterwidth,SIGNAL(valueChanged(int)),SLOT(slotConfigChanged()));
    connect(lowfilterfreq,SIGNAL(valueChanged(int)),SLOT(slotConfigChanged()));
    connect(highfilterfreq,SIGNAL(valueChanged(int)),SLOT(slotConfigChanged()));

    // Vorbis
    connect(set_vorbis_min_br,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(set_vorbis_max_br,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(set_vorbis_nominal_br,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(vorbis_min_br,SIGNAL(activated(int)),SLOT(slotConfigChanged()));
    connect(vorbis_max_br,SIGNAL(activated(int)),SLOT(slotConfigChanged()));
    connect(vorbis_nominal_br,SIGNAL(activated(int)),SLOT(slotConfigChanged()));
    connect(vorbis_enc_method,SIGNAL(activated(int)),this,SLOT(slotSelectVorbisMethod(int)));
    connect(vorbis_quality,SIGNAL(valueChanged(double)),this,SLOT(slotConfigChanged()));
    connect(vorbis_comments, SIGNAL( clicked ()),SLOT( slotConfigChanged()));

    // File Name
    connect(fileNameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotConfigChanged()));
}

KAudiocdModule::~KAudiocdModule()
{
    delete config;
}

void KAudiocdModule::defaults() {
  cd_autosearch_check->setChecked(true);
  cd_device_string->setText("/dev/cdrom");

  ec_enable_check->setChecked(true);
  ec_skip_check->setChecked(false);

  enc_method->setCurrentItem(0);
  slotSelectMethod(0);

  stereo->setCurrentItem(0);
  quality->setValue(2);

  copyright->setChecked(false);
  original->setChecked(true);
  iso->setChecked(false);
  id3_tag->setChecked(true);
  crc->setChecked(false);

  cbr_bitrate->setCurrentItem(9);

  vbr_min_br->setChecked(false);
  vbr_min_hard->setChecked(false);
  vbr_max_br->setChecked(false);
  vbr_average_br->setChecked(false);

  vbr_min_brate->setCurrentItem(7);
  vbr_max_brate->setCurrentItem(13);
  vbr_mean_brate->setCurrentItem(10);

  vbr_xing_tag->setChecked(true);

  slotUpdateVBRWidgets();

  enable_lowpass->setChecked(false);
  enable_highpass->setChecked(false);
  set_lpf_width->setChecked(false);
  set_hpf_width->setChecked(false);

  lowfilterfreq->setValue(18000);
  lowfilterwidth->setValue(900);
  highfilterfreq->setValue(0);
  highfilterwidth->setValue(0);

  slotChangeFilter();

  set_vorbis_min_br->setChecked(false);
  set_vorbis_max_br->setChecked(false);
  set_vorbis_nominal_br->setChecked(true);

  vorbis_min_br->setCurrentItem(0);
  vorbis_max_br->setCurrentItem(13);
  vorbis_nominal_br->setCurrentItem(1);
  vorbis_quality->setValue(3.0);
  vorbis_comments->setChecked(true);
  vorbis_enc_method->setCurrentItem(0);
  slotSelectVorbisMethod(0);

  fileNameLineEdit->setText("%n %t");
}

void KAudiocdModule::save() {

  if (!configChanged ) return;

  int encmethod = enc_method->currentItem();
  int mode = stereo->currentItem();
  int encquality = quality->value();

  int cbrbrate = cbr_bitrate->currentItem();
  cbrbrate = bitrates[cbrbrate];

  int vbrminbrate = vbr_min_brate->currentItem();
  vbrminbrate = bitrates[vbrminbrate];

  int vbrmaxbrate = vbr_max_brate->currentItem();
  vbrmaxbrate = bitrates[vbrmaxbrate];

  int vbravrbrate = vbr_mean_brate->currentItem();
  vbravrbrate = bitrates[vbravrbrate];

  int lpf_freq = lowfilterfreq->value();
  int lpf_width = lowfilterwidth->value();

  int hpf_freq = highfilterfreq->value();
  int hpf_width = highfilterwidth->value();

  int vorbis_min_bitrate = vorbis_min_br->currentItem();
  vorbis_min_bitrate = vorbis_bitrates[vorbis_min_bitrate];

  int vorbis_max_bitrate = vorbis_max_br->currentItem();
  vorbis_max_bitrate = vorbis_bitrates[vorbis_max_bitrate];

  int vorbis_nominal_bitrate = vorbis_nominal_br->currentItem();
  vorbis_nominal_bitrate = vorbis_nominal_bitrates[vorbis_nominal_bitrate];

  {
    KConfigGroupSaver saver(config, "CDDA");

    config->writeEntry("autosearch",cd_autosearch_check->isChecked());
    config->writeEntry("device",cd_device_string->text());
    config->writeEntry("disable_paranoia",!(ec_enable_check->isChecked()));
    config->writeEntry("never_skip",!(ec_skip_check->isChecked()));
  }
  
  {
    KConfigGroupSaver saver(config, "MP3");

    config->writeEntry("mode",mode);
    config->writeEntry("quality",encquality);
    config->writeEntry("encmethod",encmethod);
    
    config->writeEntry("copyright",copyright->isChecked());
    config->writeEntry("original",original->isChecked());
    config->writeEntry("iso",iso->isChecked());
    config->writeEntry("crc",crc->isChecked());
    config->writeEntry("id3",id3_tag->isChecked());
    
    config->writeEntry("cbrbitrate",cbrbrate);
    
    config->writeEntry("set_vbr_min",vbr_min_br->isChecked());
    config->writeEntry("set_vbr_max",vbr_max_br->isChecked());
    config->writeEntry("set_vbr_avr",vbr_average_br->isChecked());
    config->writeEntry("vbr_min_hard",vbr_min_hard->isChecked());
    config->writeEntry("vbr_min_bitrate",vbrminbrate);
    config->writeEntry("vbr_max_bitrate",vbrmaxbrate);
    config->writeEntry("vbr_average_bitrate",vbravrbrate);
    config->writeEntry("write_xing_tag",vbr_xing_tag->isChecked());
    
    config->writeEntry("enable_lowpassfilter",enable_lowpass->isChecked());
    config->writeEntry("enable_highpassfilter",enable_highpass->isChecked());
    config->writeEntry("set_highpassfilter_width",set_hpf_width->isChecked());
    config->writeEntry("set_lowpassfilter_width",set_lpf_width->isChecked());
    config->writeEntry("lowpassfilter_freq",lpf_freq);
    config->writeEntry("lowpassfilter_width",lpf_width);
    config->writeEntry("highpassfilter_freq",hpf_freq);
    config->writeEntry("highpassfilter_width",hpf_width);
  }

  {
    KConfigGroupSaver saver(config, "Vorbis");

    config->writeEntry("set_vorbis_min_bitrate",set_vorbis_min_br->isChecked());
    config->writeEntry("set_vorbis_max_bitrate",set_vorbis_max_br->isChecked());
    config->writeEntry("set_vorbis_nominal_bitrate",set_vorbis_nominal_br->isChecked());
    config->writeEntry("vorbis_comments",vorbis_comments->isChecked());
    config->writeEntry("vorbis_min_bitrate",vorbis_min_bitrate);
    config->writeEntry("vorbis_max_bitrate",vorbis_max_bitrate);
    config->writeEntry("vorbis_nominal_bitrate",vorbis_nominal_bitrate);
    config->writeEntry("encmethod", vorbis_enc_method->currentItem());
    config->writeEntry("quality", vorbis_quality->value());
  }

  {
    KConfigGroupSaver saver(config, "FileName");
    config->writeEntry("file_name_template", fileNameLineEdit->text());
  }

  config->sync();

  configChanged = false;

}

void KAudiocdModule::load() {

  {
    KConfigGroupSaver saver(config, "CDDA");

    cd_autosearch_check->setChecked(config->readBoolEntry("autosearch",true));
    cd_device_string->setText(config->readEntry("device","/dev/cdrom"));
    ec_enable_check->setChecked(!(config->readBoolEntry("disable_paranoia",false)));
    ec_skip_check->setChecked(!(config->readBoolEntry("never_skip",true)));
  }

  int brate;

  {
    KConfigGroupSaver saver(config, "MP3");
    
    int encmethod = config->readNumEntry("encmethod",0);
    enc_method->setCurrentItem(encmethod);
    slotSelectMethod(encmethod);
    
    stereo->setCurrentItem(config->readNumEntry("mode",0));
    quality->setValue(config->readNumEntry("quality",2));

    copyright->setChecked(config->readBoolEntry("copyright",false));
    original->setChecked(config->readBoolEntry("original",true));
    iso->setChecked(config->readBoolEntry("iso",false));
    crc->setChecked(config->readBoolEntry("crc",false));
    id3_tag->setChecked(config->readBoolEntry("id3",true));
    
    
    brate = config->readNumEntry("cbrbitrate",160);
    cbr_bitrate->setCurrentItem(getBitrateIndex(brate));
    
    vbr_min_br->setChecked(config->readBoolEntry("set_vbr_min",false));
    vbr_min_hard->setChecked(config->readBoolEntry("vbr_min_hard",false));
    vbr_max_br->setChecked(config->readBoolEntry("set_vbr_max",false));
    vbr_average_br->setChecked(config->readBoolEntry("set_vbr_avr",true));
    
    brate = config->readNumEntry("vbr_min_bitrate",40);
    vbr_min_brate->setCurrentItem(getBitrateIndex(brate));
    brate = config->readNumEntry("vbr_max_bitrate",320);
    vbr_max_brate->setCurrentItem(getBitrateIndex(brate));
    brate = config->readNumEntry("vbr_average_bitrate",160);
    vbr_mean_brate->setCurrentItem(getBitrateIndex(brate));
    
    vbr_xing_tag->setChecked(config->readBoolEntry("write_xing_tag",true));
    
    slotUpdateVBRWidgets();
    
    enable_lowpass->setChecked(config->readBoolEntry("enable_lowpassfilter",false));
    enable_highpass->setChecked(config->readBoolEntry("enable_highpassfilter",false));
    set_lpf_width->setChecked(config->readBoolEntry("set_lowpassfilter_width",false));
    set_hpf_width->setChecked(config->readBoolEntry("set_highpassfilter_width",false));
    
    lowfilterfreq->setValue(config->readNumEntry("lowpassfilter_freq",18000));
    lowfilterwidth->setValue(config->readNumEntry("lowpassfilter_width",900));
    highfilterfreq->setValue(config->readNumEntry("highpassfilter_freq",0));
    highfilterwidth->setValue(config->readNumEntry("highpassfilter_width",0));

    slotChangeFilter();
  }

  {
    KConfigGroupSaver saver(config, "Vorbis");

    brate = config->readNumEntry("vorbis_min_bitrate",40);
    vorbis_min_br->setCurrentItem(getVorbisBitrateIndex(brate));
    
    brate = config->readNumEntry("vorbis_max_bitrate",350);
    vorbis_max_br->setCurrentItem(getVorbisBitrateIndex(brate));
    
    brate = config->readNumEntry("vorbis_nominal_bitrate",160);
    vorbis_nominal_br->setCurrentItem(getVorbisNominalBitrateIndex(brate));
    
    set_vorbis_min_br->setChecked(config->readBoolEntry("set_vorbis_min_bitrate",false));
    set_vorbis_max_br->setChecked(config->readBoolEntry("set_vorbis_max_bitrate",false));
    set_vorbis_nominal_br->setChecked(config->readBoolEntry("set_vorbis_nominal_bitrate",true));
    
    int vorbis_encmethod = config->readNumEntry("encmethod", 0);
    vorbis_enc_method->setCurrentItem(vorbis_encmethod);
    slotSelectVorbisMethod(vorbis_encmethod);
    
    vorbis_quality->setValue(config->readDoubleNumEntry("quality", 3.0));
    vorbis_comments->setChecked(config->readBoolEntry("vorbis_comments",true));
  }

  {
    KConfigGroupSaver saver(config, "FileName");
    fileNameLineEdit->setText(config->readEntry("file_name_template", "%n %t"));
  }
}

int KAudiocdModule::getBitrateIndex(int value) {

  for (uint i=0;i < sizeof(bitrates);i++)
    if (value == bitrates[i])
      return i;
  return -1;
}

int KAudiocdModule::getVorbisBitrateIndex(int value) {

  for (uint i=0;i < sizeof(vorbis_bitrates);i++)
    if (value == vorbis_bitrates[i])
      return i;
  return -1;
}

int KAudiocdModule::getVorbisNominalBitrateIndex(int value) {

  for (uint i=0;i < sizeof(vorbis_nominal_bitrates);i++)
    if (value == vorbis_nominal_bitrates[i])
      return i;
  return -1;
}

void KAudiocdModule::slotConfigChanged() {

  if (!configChanged) configChanged = true;
  emit changed(true);
  return;
}

/*
#    slot for the error correction settings
*/
void KAudiocdModule::slotEcEnable() {

  if (!(ec_skip_check->isChecked())) {
    ec_skip_check->setChecked(true);
  } else {
    if (ec_skip_check->isEnabled()) {
      ec_skip_check->setChecked(false);
    }
  }

  slotConfigChanged();

}

//
// slot for the filter settings
//
void KAudiocdModule::slotChangeFilter() {

  if (enable_lowpass->isChecked()) {
    lowfilterfreq->setEnabled(true);
    //lowfilterwidth->setEnabled(true);
    set_lpf_width->setEnabled(true);
  } else {
    lowfilterfreq->setDisabled(true);
    lowfilterwidth->setDisabled(true);
    set_lpf_width->setChecked(false);
    set_lpf_width->setDisabled(true);

  }

  if (enable_highpass->isChecked()) {
    highfilterfreq->setEnabled(true);
    //    highfilterwidth->setEnabled(true);
    set_hpf_width->setEnabled(true);
  } else {
    highfilterfreq->setDisabled(true);
    highfilterwidth->setDisabled(true);
    set_hpf_width->setChecked(false);
    set_hpf_width->setDisabled(true);

  }

  if (set_lpf_width->isChecked()) {
    lowfilterwidth->setEnabled(true);
  } else {
     lowfilterwidth->setDisabled(true);
  }

  if (set_hpf_width->isChecked()) {
    highfilterwidth->setEnabled(true);
  } else {
     highfilterwidth->setDisabled(true);
  }

  slotConfigChanged();

}

//
// slot for switching between quality and bitrate based encoding
//
void KAudiocdModule::slotSelectVorbisMethod(int index) {
  if (index == 1 ) {
    // bitrate based encoding selected
    vorbis_bitrate_settings->show();
    vorbis_quality_settings->hide();
  } else {
    // quality based encoding selected
    vorbis_bitrate_settings->hide();
    vorbis_quality_settings->show();
  }
  slotConfigChanged();
  return;
}


//
// slot for switching between CBR and VBR
//
void KAudiocdModule::slotSelectMethod(int index) {
  if (index == 1 ) {
    // variable bitrate selected
    vbr_settings->show();
    cbr_settings->hide();
  } else {
    // constant bitrate selected
    vbr_settings->hide();
    cbr_settings->show();
  }
  slotConfigChanged();
  return;
}


//
// slot for changing the VBR Widgets logically
//
void KAudiocdModule::slotUpdateVBRWidgets() {

  if(vbr_average_br->isEnabled() && vbr_average_br->isChecked()) {

    vbr_min_br->setChecked(false);
    vbr_min_br->setDisabled(true);
    vbr_min_brate->setDisabled(true);

    vbr_min_hard->setChecked(false);

    vbr_max_br->setChecked(false);
    vbr_max_br->setDisabled(true);
    vbr_max_brate->setDisabled(true);

    vbr_mean_brate->setEnabled(true);

  } else {

    vbr_min_br->setEnabled(true);
    vbr_max_br->setEnabled(true);

    bool usingMinMaxBitrate = vbr_min_br->isChecked() || vbr_max_br->isChecked();
    vbr_average_br->setEnabled(!usingMinMaxBitrate);

    vbr_mean_brate->setDisabled(true);

    vbr_min_brate->setEnabled(vbr_min_br->isChecked());
    vbr_max_brate->setEnabled(vbr_max_br->isChecked());
  }

  slotConfigChanged();

  return;
}

QString KAudiocdModule::quickHelp() const
{
  return i18n("<h1>Audio CDs</h1> The Audio CD IO-Slave enables you to easily"
              " create wav, MP3 or Ogg Vorbis files from your audio CD-ROMs or DVDs."
              " The slave is invoked by typing <i>\"audiocd:/\"</i> in Konqueror's location"
              " bar. In this module, you can configure"
              " encoding, and device settings. Note that MP3 and Ogg"
              " Vorbis encoding are only available if KDE was built with a recent"
              " version of the LAME or Ogg Vorbis libraries.");
}

const KAboutData* KAudiocdModule::aboutData() const
{

    KAboutData *about =
    new KAboutData(I18N_NOOP("kcmaudiocd"), I18N_NOOP("KDE Audio-CD Slave Control Module"),
                   0, 0, KAboutData::License_GPL,
                   I18N_NOOP("(c) 2000 - 2001 Carsten Duvenhorst"));

    about->addAuthor("Carsten Duvenhorst", 0, "duvenhorst@duvnet.de");

    return about;
}

extern "C"
{
    KCModule *create_audiocd(QWidget *parent, const char */*name*/)
    {
        return new KAudiocdModule(parent, "kcmaudiocd");
    }

}
