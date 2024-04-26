// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicefinder.h"
#include "devicehandler.h"
#include "deviceinfo.h"

#include <QtBluetooth/qbluetoothdeviceinfo.h>

#if QT_CONFIG(permissions)
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>
#endif

DeviceFinder::DeviceFinder(DeviceHandler* handler, QObject* parent)
    : BluetoothBaseClass(parent)
    , deviceHandler_(handler) {
    //! [devicediscovery-1]
    deviceDiscoveryAgent_ = new QBluetoothDeviceDiscoveryAgent(this);
    deviceDiscoveryAgent_->setLowEnergyDiscoveryTimeout(15000);

    connect(deviceDiscoveryAgent_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceFinder::addDevice);
    connect(deviceDiscoveryAgent_, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &DeviceFinder::scanError);

    connect(deviceDiscoveryAgent_, &QBluetoothDeviceDiscoveryAgent::finished, this, &DeviceFinder::scanFinished);
    connect(deviceDiscoveryAgent_, &QBluetoothDeviceDiscoveryAgent::canceled, this, &DeviceFinder::scanFinished);
    //! [devicediscovery-1]

    resetMessages();
}

DeviceFinder::~DeviceFinder() {
    qDeleteAll(devices_);
    devices_.clear();
}

void DeviceFinder::startSearch() {
#if QT_CONFIG(permissions)
    //! [permissions]
    QBluetoothPermission permission{};
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch(qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, &DeviceFinder::startSearch);
        return;
    case Qt::PermissionStatus::Denied:
        setError(tr("Bluetooth permissions not granted!"));
        setIcon(IconError);
        return;
    case Qt::PermissionStatus::Granted:
        break; // proceed to search
    }
    //! [permissions]
#endif // QT_CONFIG(permissions)
    clearMessages();
    deviceHandler_->setDevice(nullptr);
    qDeleteAll(devices_);
    devices_.clear();

    emit devicesChanged();

    //! [devicediscovery-2]
    deviceDiscoveryAgent_->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    //! [devicediscovery-2]

    emit scanningChanged();
    setInfo(tr("Scanning for devices..."));
    setIcon(IconProgress);
}

void DeviceFinder::stopSearch() {
    if(deviceDiscoveryAgent_) deviceDiscoveryAgent_->stop();
}

//! [devicediscovery-3]
void DeviceFinder::addDevice(const QBluetoothDeviceInfo& device) {
    // If device is LowEnergy-device, add it to the list
    if(device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        auto devInfo = new DeviceInfo(device);
        auto it = std::find_if(devices_.begin(), devices_.end(),
            [devInfo](DeviceInfo* dev) {
                return devInfo->getAddress() == dev->getAddress();
            });
        if(it == devices_.end()) {
            devices_.append(devInfo);
        } else {
            auto oldDev = *it;
            *it = devInfo;
            delete oldDev;
        }
        setInfo(tr("Low Energy device found. Scanning more..."));
        setIcon(IconProgress);
        //! [devicediscovery-3]
        emit devicesChanged();
        //! [devicediscovery-4]
    }
    //...
}
//! [devicediscovery-4]

void DeviceFinder::scanError(QBluetoothDeviceDiscoveryAgent::Error error) {
    if(error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        setError(tr("The Bluetooth adaptor is powered off."));
    else if(error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        setError(tr("Writing or reading from the device resulted in an error."));
    else
        setError(tr("An unknown error has occurred."));
    setIcon(IconError);
}

void DeviceFinder::scanFinished() {
    if(devices_.isEmpty()) {
        setError(tr("No Low Energy devices found."));
        setIcon(IconError);
    } else {
        setInfo(tr("Scanning done."));
        setIcon(IconBluetooth);
    }

    emit scanningChanged();
    emit devicesChanged();
}

void DeviceFinder::resetMessages() {
    setError("");
    setInfo(tr("Start search to find devices"));
    setIcon(IconSearch);
}

void DeviceFinder::connectToService(const QString& address) {
    deviceDiscoveryAgent_->stop();

    DeviceInfo* currentDevice = nullptr;
    for(QObject* entry: std::as_const(devices_)) {
        auto device = qobject_cast<DeviceInfo*>(entry);
        if(device && device->getAddress() == address) {
            currentDevice = device;
            break;
        }
    }

    if(currentDevice)
        deviceHandler_->setDevice(currentDevice);

    resetMessages();
}

bool DeviceFinder::scanning() const {
    return deviceDiscoveryAgent_->isActive();
}

QVariant DeviceFinder::devices() {
    return QVariant::fromValue(devices_);
}
