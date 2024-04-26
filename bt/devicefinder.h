// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEVICEFINDER_H
#define DEVICEFINDER_H

#include "bluetoothbaseclass.h"

#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>

#include <QtCore/qtimer.h>
#include <QtCore/qvariant.h>

#include <QtQmlIntegration/qqmlintegration.h>

QT_BEGIN_NAMESPACE
class QBluetoothDeviceInfo;
QT_END_NAMESPACE

class DeviceInfo;
class DeviceHandler;

class DeviceFinder : public BluetoothBaseClass {
    Q_OBJECT

    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QVariant devices READ devices NOTIFY devicesChanged)

    QML_ELEMENT
    QML_UNCREATABLE("This class is not intended to be created directly")

public:
    DeviceFinder(DeviceHandler* handler, QObject* parent = nullptr);
    ~DeviceFinder();

    bool scanning() const;
    QVariant devices();
    auto devices2() {
        return devices_;
    }

public slots:
    void startSearch();
    void stopSearch();
    void connectToService(const QString& address);

private slots:
    void addDevice(const QBluetoothDeviceInfo& device);
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void scanFinished();

signals:
    void scanningChanged();
    void devicesChanged();

private:
    void resetMessages();

    DeviceHandler* deviceHandler_;
    QBluetoothDeviceDiscoveryAgent* deviceDiscoveryAgent_;
    QList<DeviceInfo*> devices_;

    QTimer demoTimer_;
};

#endif // DEVICEFINDER_H
