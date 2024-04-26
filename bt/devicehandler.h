// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include "bluetoothbaseclass.h"

#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergyservice.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qlist.h>
#include <QtCore/qtimer.h>

#include <QtQml/qqmlregistration.h>

class DeviceInfo;

class DeviceHandler : public BluetoothBaseClass {
    Q_OBJECT

    Q_PROPERTY(bool runing READ runing NOTIFY runingChanged)
    Q_PROPERTY(bool alive READ alive NOTIFY aliveChanged)
    Q_PROPERTY(AddressType addressType READ addressType WRITE setAddressType)

    QML_ELEMENT

public:
    enum class AddressType {
        PublicAddress,
        RandomAddress
    };
    Q_ENUM(AddressType)

    DeviceHandler(QObject* parent = nullptr);

    void setDevice(DeviceInfo* device);
    void setAddressType(AddressType type);
    AddressType addressType() const;

    bool runing() const;
    bool alive() const;

    bool write(const QByteArray& hex);

signals:
    void runingChanged();
    void aliveChanged();

public slots:
    void startMeasurement();
    void stopMeasurement();
    void disconnectService();

private:
    // QLowEnergyController
    void serviceDiscovered(const QBluetoothUuid&);
    void serviceScanDone();

    // QLowEnergyService
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateCharacteristicValue(const QLowEnergyCharacteristic& c, const QByteArray& value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor& d, const QByteArray& value);

private:
    QLowEnergyController* control_ = nullptr;
    QLowEnergyService* service_ = nullptr;
    QLowEnergyCharacteristic characteristicControl_;
    QLowEnergyDescriptor notificationDescControl_;
    QLowEnergyDescriptor notificationDescReceive_;
    DeviceInfo* currentDevice_ = nullptr;

    bool foundHeartRateService_ = false;
    bool runing_ = false;

    QLowEnergyController::RemoteAddressType addressType_ = QLowEnergyController::PublicAddress;
};

#endif // DEVICEHANDLER_H
