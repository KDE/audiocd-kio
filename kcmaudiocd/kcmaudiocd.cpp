/*
   Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>
   Copyright (C) 2005 Benjamin Meyer <ben at meyerhome dot net>
  
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kinstance.h>
#include <qcheckbox.h>
#include <q3combobox.h>
#include <q3groupbox.h>
#include <qslider.h>
#include <qtabwidget.h>
#include <kaboutdata.h>
#include <knuminput.h>
#include <qregexp.h>
#include <qlabel.h>

#include <audiocdencoder.h>
#include "kcmaudiocd.moc"
#include <kconfigdialogmanager.h>

KAudiocdModule::KAudiocdModule(KInstance *instance, QWidget *parent, const QStringList &lst)
  : KCModule(instance, parent, lst), configChanged(false)
{
	audioConfig = new AudiocdConfig(parent,"");
    QString foo = i18n("Report errors found on the cd.");

    setButtons(Default|Apply);

    config = new KConfig("kcmaudiocdrc");

    QList<AudioCDEncoder*> encoders;
    AudioCDEncoder::findAllPlugins(0, encoders);
    foreach (AudioCDEncoder *encoder, encoders) {
      KConfigSkeleton *config = NULL;
      QWidget *widget = encoder->getConfigureWidget(&config);
      if(widget && config){
	 audioConfig->tabWidget->addTab(widget, i18n("%1 Encoder").arg(encoder->type()));
         KConfigDialogManager *configManager = new KConfigDialogManager(widget, config);
         encoderSettings.append(configManager);
      }
    }

    qDeleteAll(encoders);
    encoders.clear();

    load();
  
    KConfigDialogManager *widget;
    for ( widget = encoderSettings.first(); widget; widget = encoderSettings.next() ){
      connect(widget, SIGNAL(widgetModified()), this, SLOT(slotModuleChanged()));
    }
   
    //CDDA Options
    connect(audioConfig->cd_autosearch_check,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(audioConfig->ec_enable_check,SIGNAL(clicked()),this,SLOT(slotEcEnable()));
    connect(audioConfig->ec_skip_check,SIGNAL(clicked()),SLOT(slotConfigChanged()));
    connect(audioConfig->cd_device_string,SIGNAL(textChanged(const QString &)),SLOT(slotConfigChanged()));

    // File Name
    connect(audioConfig->fileNameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotConfigChanged()));
    connect(audioConfig->albumNameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotConfigChanged()));
    connect( audioConfig->kcfg_replaceInput, SIGNAL( textChanged(const QString&) ), this, SLOT( updateExample() ) );
    connect( audioConfig->kcfg_replaceOutput, SIGNAL( textChanged(const QString&) ), this, SLOT( updateExample() ) );
    connect( audioConfig->example, SIGNAL( textChanged(const QString&) ), this, SLOT( updateExample() ) );

    KAboutData *about =
    new KAboutData(I18N_NOOP("kcmaudiocd"), I18N_NOOP("KDE Audio CD IO Slave"),
                   0, 0, KAboutData::License_GPL,
                   I18N_NOOP("(c) 2000 - 2005 Audio CD developers"));

    about->addAuthor("Benjamin C. Meyer", I18N_NOOP("Current Maintainer"), "ben@meyerhome.net");
    about->addAuthor("Carsten Duvenhorst", 0, "duvenhorst@duvnet.de");
    setAboutData(about);
}

KAudiocdModule::~KAudiocdModule()
{
    delete config;
}

void KAudiocdModule::updateExample()
{
  QString text = audioConfig->example->text();
  text.replace( QRegExp(audioConfig->kcfg_replaceInput->text()), audioConfig->kcfg_replaceOutput->text() );
  audioConfig->exampleOutput->setText(text);
}

void KAudiocdModule::defaults() {
	audioConfig->cd_autosearch_check->setChecked(true);
	audioConfig->cd_device_string->setText("/dev/cdrom");

	audioConfig->ec_enable_check->setChecked(true);
	audioConfig->ec_skip_check->setChecked(false);
	audioConfig->niceLevel->setValue(0);
	
	audioConfig->kcfg_replaceInput->setText("");
	audioConfig->kcfg_replaceOutput->setText("");
	audioConfig->example->setText(i18n("Cool artist - example audio file.wav"));
	KConfigDialogManager *widget;
	for ( widget = encoderSettings.first(); widget; widget = encoderSettings.next() ){
		widget->updateWidgetsDefault();
	}

	audioConfig->fileNameLineEdit->setText("%{trackartist} - %{number} - %{title}");
	audioConfig->albumNameLineEdit->setText("%{albumartist} - %{albumtitle}");
}

void KAudiocdModule::save() {
  if (!configChanged ) return;

  {
    KConfigGroup cg(config, "CDDA");

    cg.writeEntry("autosearch",audioConfig->cd_autosearch_check->isChecked());
    cg.writeEntry("device",audioConfig->cd_device_string->text());
    cg.writeEntry("disable_paranoia",!(audioConfig->ec_enable_check->isChecked()));
    cg.writeEntry("never_skip",!(audioConfig->ec_skip_check->isChecked()));
    cg.writeEntry("niceLevel", audioConfig->niceLevel->value());
  }
  
  {
    KConfigGroup cg(config, "FileName");
    cg.writeEntry("file_name_template", audioConfig->fileNameLineEdit->text());
    cg.writeEntry("album_name_template", audioConfig->albumNameLineEdit->text());
    cg.writeEntry("regexp_search",audioConfig->kcfg_replaceInput->text());
    cg.writeEntry("regexp_replace", audioConfig->kcfg_replaceOutput->text());
    cg.writeEntry("regexp_example", audioConfig->example->text());
  }

  KConfigDialogManager *widget;
  for ( widget = encoderSettings.first(); widget; widget = encoderSettings.next() ){
    widget->updateSettings();
  }
  
  config->sync();

  configChanged = false;

}

void KAudiocdModule::load() {

  {
    KConfigGroup cg(config, "CDDA");

    audioConfig->cd_autosearch_check->setChecked(cg.readBoolEntry("autosearch",true));
    audioConfig->cd_device_string->setText(cg.readEntry("device","/dev/cdrom"));
    audioConfig->ec_enable_check->setChecked(!(cg.readBoolEntry("disable_paranoia",false)));
    audioConfig->ec_skip_check->setChecked(!(cg.readBoolEntry("never_skip",true)));
    audioConfig->niceLevel->setValue(cg.readNumEntry("niceLevel", 0));
  }

  {
    KConfigGroup cg(config, "FileName");
    audioConfig->fileNameLineEdit->setText(cg.readEntry("file_name_template", "%{trackartist} - %{number} - %{title}"));
    audioConfig->albumNameLineEdit->setText(cg.readEntry("album_name_template", "%{albumartist} - %{albumtitle}"));
    audioConfig->kcfg_replaceInput->setText(cg.readEntry("regexp_search"));
    audioConfig->kcfg_replaceOutput->setText(cg.readEntry("regexp_replace"));
    audioConfig->example->setText(cg.readEntry("example", i18n("Cool artist - example audio file.wav")));
  }
  
  KConfigDialogManager *widget;
  for ( widget = encoderSettings.first(); widget; widget = encoderSettings.next() ){
    widget->updateWidgets();
  }
}

void KAudiocdModule::slotModuleChanged() {
	KConfigDialogManager *widget;
	for ( widget = encoderSettings.first(); widget; widget = encoderSettings.next() ){
		if(widget->hasChanged()){
			slotConfigChanged();
			break;
		}
	}
}

void KAudiocdModule::slotConfigChanged() {
	configChanged = true;
	emit changed(true);
}

/*
#    slot for the error correction settings
*/
void KAudiocdModule::slotEcEnable() {
  if (!(audioConfig->ec_skip_check->isChecked())) {
    audioConfig->ec_skip_check->setChecked(true);
  } else {
    if (audioConfig->ec_skip_check->isEnabled()) {
      audioConfig->ec_skip_check->setChecked(false);
    }
  }

  slotConfigChanged();
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

extern "C"
{
    KCModule *create_audiocd(QWidget *parent, const char */*name*/)
    {
		KInstance *inst = new KInstance("kcmaudiocd");
        return new KAudiocdModule(inst,parent);
    }

}
