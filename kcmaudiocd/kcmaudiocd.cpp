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

#include "kcmaudiocd.h"
#include "audiocdplugins_version.h"

#include <QCheckBox>
#include <QLabel>
#include <QRegExp>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

#include <KAboutData>
#include <KConfigDialogManager>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginLoader>

#include <audiocdencoder.h>

K_PLUGIN_FACTORY(Factory,
        registerPlugin<KAudiocdModule>();
        )

KAudiocdModule::KAudiocdModule(QWidget *parent, const QVariantList &lst)
  : KCModule(parent), configChanged(false)
{
    Q_UNUSED(lst);
    
    QVBoxLayout *box = new QVBoxLayout(this);

    audioConfig = new AudiocdConfig(this);

    box->addWidget(audioConfig);
    setButtons(Default|Apply);

    config = new KConfig( QLatin1String( "kcmaudiocdrc" ));

    QList<AudioCDEncoder*> encoders;
    AudioCDEncoder::findAllPlugins(0, encoders);
    foreach (AudioCDEncoder *encoder, encoders) {
      if (encoder->init()) {
        KConfigSkeleton *config = NULL;
        QWidget *widget = encoder->getConfigureWidget(&config);
        if(widget && config){
  	   audioConfig->tabWidget->addTab(widget, i18n("%1 Encoder", encoder->type()));
           KConfigDialogManager *configManager = new KConfigDialogManager(widget, config);
           encoderSettings.append(configManager);
        }
      }
    }

    qDeleteAll(encoders);
    encoders.clear();


    for (int i = 0; i < encoderSettings.size(); ++i) {
        connect( encoderSettings.at( i ), &KConfigDialogManager::widgetModified, this, &KAudiocdModule::slotModuleChanged);
    }

    //CDDA Options
    connect(audioConfig->ec_enable_check, &QCheckBox::clicked, this, &KAudiocdModule::slotEcEnable);
    connect(audioConfig->ec_skip_check, &QCheckBox::clicked, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->niceLevel, &QSlider::valueChanged, this, &KAudiocdModule::slotConfigChanged);

    // File Name
    connect(audioConfig->fileNameLineEdit, &QLineEdit::textChanged, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->albumNameLineEdit, &QLineEdit::textChanged, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->fileLocationLineEdit, &QLineEdit::textChanged, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->fileLocationGroupBox, &QGroupBox::clicked, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->kcfg_replaceInput, &QLineEdit::textChanged, this, &KAudiocdModule::updateExample);
    connect(audioConfig->kcfg_replaceOutput, &QLineEdit::textChanged, this, &KAudiocdModule::updateExample);
    connect(audioConfig->example, &QLineEdit::textChanged, this, &KAudiocdModule::updateExample);
    connect(audioConfig->kcfg_replaceInput, &QLineEdit::textChanged, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->kcfg_replaceOutput, &QLineEdit::textChanged, this, &KAudiocdModule::slotConfigChanged);
    connect(audioConfig->example, &QLineEdit::textChanged, this, &KAudiocdModule::slotConfigChanged);


    KAboutData *about =
    new KAboutData("kcmaudiocd", i18n("KDE Audio CD IO Slave"), AUDIOCDPLUGINS_VERSION_STRING);

    about->addAuthor(i18n("Benjamin C. Meyer"), i18n("Former Maintainer"), "ben@meyerhome.net");
    about->addAuthor(i18n("Carsten Duvenhorst"), i18n("Original Author"), "duvenhorst@duvnet.de");
    setAboutData(about);
}

KAudiocdModule::~KAudiocdModule()
{
    delete config;
}

QString removeQoutes(const QString& text)
{
   QString deqoutedString=text;
   QRegExp qoutedStringRegExp( QLatin1String( "^\".*\"$" ));
   if (qoutedStringRegExp.exactMatch(text))
   {
      deqoutedString=text.mid(1, text.length()-2);
   }
   return deqoutedString;
}

bool needsQoutes(const QString& text)
{
   QRegExp spaceAtTheBeginning( QLatin1String( "^\\s+.*$" ));
   QRegExp spaceAtTheEnd( QLatin1String( "^.*\\s+$" ));
   return (spaceAtTheBeginning.exactMatch(text) || spaceAtTheEnd.exactMatch(text));
}

void KAudiocdModule::updateExample()
{
  QString text = audioConfig->example->text();

  QString deqoutedReplaceInput=removeQoutes(audioConfig->kcfg_replaceInput->text());
  QString deqoutedReplaceOutput=removeQoutes(audioConfig->kcfg_replaceOutput->text());
  text.replace( QRegExp(deqoutedReplaceInput), deqoutedReplaceOutput );
  audioConfig->exampleOutput->setText(text);
}

void KAudiocdModule::defaults() {
	audioConfig->ec_enable_check->setChecked(true);
	audioConfig->ec_skip_check->setChecked(false);
	audioConfig->niceLevel->setValue(0);

	audioConfig->kcfg_replaceInput->setText("");
	audioConfig->kcfg_replaceOutput->setText("");
	audioConfig->example->setText(i18n("Cool artist - example audio file.wav"));
        for (int i = 0; i < encoderSettings.size(); ++i) {
            encoderSettings.at( i )->updateWidgetsDefault();
        }

	audioConfig->fileNameLineEdit->setText("%{trackartist} - %{number} - %{title}");
	audioConfig->albumNameLineEdit->setText("%{albumartist} - %{albumtitle}");
}

void KAudiocdModule::save() {
  if (!configChanged ) return;

  {
    KConfigGroup cg(config, "CDDA");

    cg.writeEntry("disable_paranoia",!(audioConfig->ec_enable_check->isChecked()));
    cg.writeEntry("never_skip",!(audioConfig->ec_skip_check->isChecked()));
    cg.writeEntry("niceLevel", audioConfig->niceLevel->value());
  }

  {
    KConfigGroup cg(config, "FileName");
    cg.writeEntry("file_name_template", audioConfig->fileNameLineEdit->text());
    cg.writeEntry("album_name_template", audioConfig->albumNameLineEdit->text());
    cg.writeEntry("show_file_location", audioConfig->fileLocationGroupBox->isChecked());
    cg.writeEntry("file_location_template", audioConfig->fileLocationLineEdit->text());
    cg.writeEntry("regexp_example", audioConfig->example->text());

        // save quoted if required
    QString replaceInput=audioConfig->kcfg_replaceInput->text();
    QString replaceOutput=audioConfig->kcfg_replaceOutput->text();
    if (needsQoutes(replaceInput))
    {
       replaceInput=QString("\"")+replaceInput+QString("\"");
    }
    if (needsQoutes(replaceOutput))
    {
       replaceOutput=QString("\"")+replaceOutput+QString("\"");
    }
    cg.writeEntry("regexp_search", replaceInput);
    cg.writeEntry("regexp_replace", replaceOutput);
  }

  for ( int i = 0;i<encoderSettings.size();++i )
  {
      encoderSettings.at( i )->updateSettings();
  }


  config->sync();

  configChanged = false;

}

void KAudiocdModule::load() {

  {
    KConfigGroup cg(config, "CDDA");

    audioConfig->ec_enable_check->setChecked(!(cg.readEntry("disable_paranoia",false)));
    audioConfig->ec_skip_check->setChecked(!(cg.readEntry("never_skip",true)));
    audioConfig->niceLevel->setValue(cg.readEntry("niceLevel", 0));
  }

  {
    KConfigGroup cg(config, "FileName");
    audioConfig->fileNameLineEdit->setText(cg.readEntry("file_name_template", "%{trackartist} - %{number} - %{title}"));
    audioConfig->albumNameLineEdit->setText(cg.readEntry("album_name_template", "%{albumartist} - %{albumtitle}"));
    audioConfig->fileLocationGroupBox->setChecked(cg.readEntry("show_file_location", false));
    audioConfig->fileLocationLineEdit->setText(cg.readEntry("file_location_template", QString()));
    audioConfig->kcfg_replaceInput->setText(cg.readEntry("regexp_search"));
    audioConfig->kcfg_replaceOutput->setText(cg.readEntry("regexp_replace"));
    audioConfig->example->setText(cg.readEntry("example", i18n("Cool artist - example audio file.wav")));
  }

  for ( int i = 0;i <encoderSettings.size();++i )
  {
      encoderSettings.at( i )->updateWidgets();
  }

}

void KAudiocdModule::slotModuleChanged() {
    for ( int i = 0;i<encoderSettings.size();++i )
    {
        KConfigDialogManager *widget = encoderSettings.at( i );
        if ( widget->hasChanged() )
        {
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

#include "kcmaudiocd.moc"
