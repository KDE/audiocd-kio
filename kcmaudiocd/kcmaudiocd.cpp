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
#include <klineedit.h>
#include <klocale.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qslider.h>
#include <qtabwidget.h>
#include <kaboutdata.h>
#include <knuminput.h>
#include <qregexp.h>
#include <qlabel.h>

#include <audiocdencoder.h>
#include "kcmaudiocd.moc"
#include <kconfigdialogmanager.h>
#include <audiocdsettings.h>

KAudiocdModule::KAudiocdModule(QWidget *parent, const char *name)
  : AudiocdConfig(parent, name), configChanged(false)
{
    setButtons(Default|Apply);

    QPtrList<AudioCDEncoder> encoders;
    AudioCDEncoder::find_all_plugins(0, encoders);
    AudioCDEncoder *encoder;
    for ( encoder = encoders.first(); encoder; encoder = encoders.next() ){
      KConfigSkeleton *config = NULL;
      QWidget *widget = encoder->getConfigureWidget(&config);
      if(widget && config){
	 tabWidget->addTab(widget, i18n("%1 Encoder").arg(encoder->type()));
         KConfigDialogManager *configManager = new KConfigDialogManager(widget, config, QString(encoder->type()+" EncoderConfigManager").latin1());
         encoderSettings.append(configManager);
      }
    }

    load();

    KConfigDialogManager *widget;
    for ( widget = encoderSettings.first(); widget; widget = encoderSettings.next() ){
      connect(widget, SIGNAL(widgetModified()), this, SLOT(slotModuleChanged()));
    }

    //CDDA Options
    connect(kcfg_autoDevice,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(kcfg_correctErrors,SIGNAL(clicked()),this,SLOT(slotEcEnable()));
    connect(kcfg_skipOnError,SIGNAL(clicked()),SLOT(slotConfigChanged()));
    connect(kcfg_device,SIGNAL(textChanged(const QString &)),SLOT(slotConfigChanged()));

    // File Name
    connect(kcfg_trackTemplate, SIGNAL(textChanged(const QString &)), this, SLOT(slotConfigChanged()));
    connect(kcfg_albumTemplate, SIGNAL(textChanged(const QString &)), this, SLOT(slotConfigChanged()));
    connect( kcfg_replaceInput, SIGNAL( textChanged(const QString&) ), this, SLOT( updateExample() ) );
    connect( kcfg_replaceOutput, SIGNAL( textChanged(const QString&) ), this, SLOT( updateExample() ) );
    connect( kcfg_example, SIGNAL( textChanged(const QString&) ), this, SLOT( updateExample() ) );

    KAboutData *about =
    new KAboutData(I18N_NOOP("kcmaudiocd"), I18N_NOOP("KDE Audio CD IO Slave"),
                   0, 0, KAboutData::License_GPL,
                   I18N_NOOP("(c) 2000 - 2005 Audio CD developers"));

    about->addAuthor("Benjamin C. Meyer", I18N_NOOP("Current Maintainer"), "ben+audiocd@meyerhome.net");
    about->addAuthor("Carsten Duvenhorst", 0, "duvenhorst@duvnet.de");
    setAboutData(about);
}

KAudiocdModule::~KAudiocdModule()
{
}

void KAudiocdModule::updateExample()
{
  QString text = kcfg_example->text();
  text.replace( QRegExp(kcfg_replaceInput->text()), kcfg_replaceOutput->text() );
  exampleOutput->setText(text);
}

void KAudiocdModule::defaults()
{
    AudioCdSettings::self()->setDefaults();

    // CDDA
    kcfg_autoDevice->setChecked(AudioCdSettings::autoDevice());
    kcfg_device->setText(AudioCdSettings::device());
    kcfg_correctErrors->setChecked(!AudioCdSettings::correctErrors());
    kcfg_skipOnError->setChecked(!AudioCdSettings::skipOnError());
    kcfg_encoderPriority->setValue(AudioCdSettings::encoderPriority());

    // FileName
    kcfg_trackTemplate->setText(AudioCdSettings::trackTemplate());
    kcfg_albumTemplate->setText(AudioCdSettings::albumTemplate());
    kcfg_replaceInput->setText(AudioCdSettings::replaceInput());
    kcfg_replaceOutput->setText(AudioCdSettings::replaceOutput());
    kcfg_example->setText(AudioCdSettings::example());

    KConfigDialogManager *widget;
    for (widget = encoderSettings.first(); widget; widget = encoderSettings.next())
    {
        widget->updateWidgetsDefault();
    }
}

void KAudiocdModule::save()
{
    if (!configChanged)
        return;

    // CDDA
    AudioCdSettings::setAutoDevice(kcfg_autoDevice->isChecked());
    AudioCdSettings::setDevice(kcfg_device->text());
    AudioCdSettings::setCorrectErrors(!kcfg_correctErrors->isChecked());
    AudioCdSettings::setSkipOnError(!kcfg_skipOnError->isChecked());
    AudioCdSettings::setEncoderPriority(kcfg_encoderPriority->value());

    // FileName
    AudioCdSettings::setTrackTemplate(kcfg_trackTemplate->text());
    AudioCdSettings::setAlbumTemplate(kcfg_albumTemplate->text());
    AudioCdSettings::setReplaceInput(kcfg_replaceInput->text());
    AudioCdSettings::setReplaceOutput(kcfg_replaceOutput->text());
    AudioCdSettings::setExample(kcfg_example->text());

    AudioCdSettings::writeConfig();

    KConfigDialogManager *widget;
    for (widget = encoderSettings.first(); widget; widget = encoderSettings.next())
    {
        widget->updateSettings();
    }
    configChanged = false;
}

void KAudiocdModule::load()
{
    // CDDA
    kcfg_autoDevice->setChecked(AudioCdSettings::autoDevice());
    kcfg_device->setText(AudioCdSettings::device());
    kcfg_correctErrors->setChecked(!AudioCdSettings::correctErrors());
    kcfg_skipOnError->setChecked(!AudioCdSettings::skipOnError());
    kcfg_encoderPriority->setValue(AudioCdSettings::encoderPriority());

    // FileName
    kcfg_trackTemplate->setText(AudioCdSettings::trackTemplate());
    kcfg_albumTemplate->setText(AudioCdSettings::albumTemplate());
    kcfg_replaceInput->setText(AudioCdSettings::replaceInput());
    kcfg_replaceOutput->setText(AudioCdSettings::replaceOutput());
    kcfg_example->setText(AudioCdSettings::example());

    KConfigDialogManager *widget;
    for (widget = encoderSettings.first(); widget; widget = encoderSettings.next())
    {
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
  if (!(kcfg_skipOnError->isChecked())) {
    kcfg_skipOnError->setChecked(true);
  } else {
    if (kcfg_skipOnError->isEnabled()) {
      kcfg_skipOnError->setChecked(false);
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
        return new KAudiocdModule(parent, "kcmaudiocd");
    }

}
