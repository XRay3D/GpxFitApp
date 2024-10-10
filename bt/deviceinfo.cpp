// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "deviceinfo.h"

#include <QtBluetooth/qbluetoothaddress.h>
#include <QtBluetooth/qbluetoothuuid.h>

using namespace Qt::StringLiterals;

DeviceInfo::DeviceInfo(const QBluetoothDeviceInfo& info)
    : device_(info) {
}

QBluetoothDeviceInfo DeviceInfo::getDevice() const { return device_; }

QString DeviceInfo::getName() const {
    auto data = device_.manufacturerData();
    for(auto&& key: data.uniqueKeys())
        qCritical() << device_.name() << key << data.values(key);
    return device_.name();
}

QString DeviceInfo::getAddress() const {
#ifdef Q_OS_DARWIN
    // workaround for Core Bluetooth:
    return device_.deviceUuid().toString();
#else
    return device_.address().toString();
#endif
}

void DeviceInfo::setDevice(const QBluetoothDeviceInfo& device) {
    device_ = device;
    emit deviceChanged();
}
