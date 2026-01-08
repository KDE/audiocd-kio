/*
 * SPDX-FileCopyrightText: 2026 Nate Graham <nate@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kcmaudiocdmoduledata.h"

#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/OpticalDrive>

AudioCDModuleData::AudioCDModuleData(QObject *parent)
    : KCModuleData(parent)
{
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &AudioCDModuleData::updateRelevance);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &AudioCDModuleData::updateRelevance);

    updateRelevance();
}

void AudioCDModuleData::updateRelevance()
{
    const auto opticalDrives = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive);
    setRelevant(opticalDrives.count() > 0);
}

#include "moc_kcmaudiocdmoduledata.cpp"
