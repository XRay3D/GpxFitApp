// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QtBluetooth/qbluetoothdeviceinfo.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

class DeviceInfo: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString deviceName READ getName NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceAddress READ getAddress NOTIFY deviceChanged)

public:
    DeviceInfo(const QBluetoothDeviceInfo &device);

    void setDevice(const QBluetoothDeviceInfo &device);
    QString getName() const;
    QString getAddress() const;
    QBluetoothDeviceInfo getDevice() const;

signals:
    void deviceChanged();

private:
    QBluetoothDeviceInfo device_;
};

#endif // DEVICEINFO_H
