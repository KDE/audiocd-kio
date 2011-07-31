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
#include <kcomponentdata.h>
#include <QCheckBox>
#include <qslider.h>
#include <qtabwidget.h>
#include <kaboutdata.h>
#include <knuminput.h>
#include <QRegExp>
#include <QLabel>
#include <QVBoxLayout>
#include <kdialog.h>

#include <audiocdencoder.h>
#include "kcmaudiocd.moc"
#include <kconfigdialogmanager.h>
#include <kconfiggroup.h>
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY(KAudiocdFactory,
        registerPlugin<KAudiocdModule>();
        )
K_EXPORT_PLUGIN(KAudiocdFactory("kcmaudiocd"))

KAudiocdModule::KAudiocdModule(QWidget *parent, const QVariantList &lst)
  : KCModule(KAudiocdFactory::componentData(), parent, lst), configChanged(false)
{
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
        connect( encoderSettings.at( i ),SIGNAL(widgetModified()), this, SLOT(slotModuleChanged()));
    }

    //CDDA Options
    connect(audioConfig->cd_autosearch_check,SIGNAL(clicked()),this,SLOT(slotConfigChanged()));
    connect(audioConfig->ec_enable_check,SIGNAL(clicked()),this,SLOT(slotEcEnable()));
    connect(audioConfig->ec_skip_check,SIGNAL(clicked()),SLOT(slotConfigChanged()));
    connect(audioConfig->cd_device_string,SIGNAL(textChanged(QString)),SLOT(slotConfigChanged()));
    connect(audioConfig->niceLevel,SIGNAL(valueChanged(int)),SLOT(slotConfigChanged()));

    // File Name
    connect(audioConfig->fileNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotConfigChanged()));
    connect(audioConfig->albumNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotConfigChanged()));
    connect(audioConfig->fileLocationLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotConfigChanged()));
    connect(audioConfig->fileLocationGroupBox, SIGNAL(clicked()), this, SLOT(slotConfigChanged()));
    connect( audioConfig->kcfg_replaceInput, SIGNAL(textChanged(QString)), this, SLOT(updateExample()) );
    connect( audioConfig->kcfg_replaceOutput, SIGNAL(textChanged(QString)), this, SLOT(updateExample()) );
    connect( audioConfig->example, SIGNAL(textChanged(QString)), this, SLOT(updateExample()) );
    connect( audioConfig->kcfg_replaceInput, SIGNAL(textChanged(QString)), this, SLOT(slotConfigChanged()) );
    connect( audioConfig->kcfg_replaceOutput, SIGNAL(textChanged(QString)), this, SLOT(slotConfigChanged()) );
    connect( audioConfig->example, SIGNAL(textChanged(QString)), this, SLOT(slotConfigChanged()) );


    KAboutData *about =
    new KAboutData(I18N_NOOP("kcmaudiocd"), 0, ki18n("KDE Audio CD IO Slave"),
                   0, KLocalizedString(), KAboutData::License_GPL,
                   ki18n("(c) 2000 - 2005 Audio CD developers"));

    about->addAuthor(ki18n("Benjamin C. Meyer"), ki18n("Former Maintainer"), "ben@meyerhome.net");
    about->addAuthor(ki18n("Carsten Duvenhorst"), KLocalizedString(), "duvenhorst@duvnet.de");
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
	audioConfig->cd_autosearch_check->setChecked(true);
	audioConfig->cd_device_string->setText("/dev/cdrom");

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
    cg.writeEntry("show_file_location", audioConfig->fileLocationGroupBox->isChecked());
    cg.writeEntry("file_location_template", audioConfig->fileLocationLineEdit->text());
    cg.writeEntry("regexp_example", audioConfig->example->text());

        // save qouted if required
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

    audioConfig->cd_autosearch_check->setChecked(cg.readEntry("autosearch",true));
    audioConfig->cd_device_string->setText(cg.readEntry("device","/dev/cdrom"));
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

