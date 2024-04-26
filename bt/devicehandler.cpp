// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicehandler.h"
#include "deviceinfo.h"

#include <QtCore/qendian.h>
#include <QtCore/qrandom.h>

// 8ce5cc030a4d11e9ab14d663bd873d93
//                               "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
const QBluetoothUuid FileService{"{8ce5cc01-0a4d-11e9-ab14-d663bd873d93}"};
const QBluetoothUuid FileControlChar{"{8ce5cc02-0a4d-11e9-ab14-d663bd873d93}"};
const QBluetoothUuid FileReceiveChar{"{8ce5cc03-0a4d-11e9-ab14-d663bd873d93}"};

DeviceHandler::DeviceHandler(QObject* parent)
    : BluetoothBaseClass(parent) {
}

void DeviceHandler::setAddressType(AddressType type) {
    switch(type) {
    case DeviceHandler::AddressType::PublicAddress:
        addressType_ = QLowEnergyController::PublicAddress;
        break;
    case DeviceHandler::AddressType::RandomAddress:
        addressType_ = QLowEnergyController::RandomAddress;
        break;
    }
}

DeviceHandler::AddressType DeviceHandler::addressType() const {
    if(addressType_ == QLowEnergyController::RandomAddress)
        return DeviceHandler::AddressType::RandomAddress;

    return DeviceHandler::AddressType::PublicAddress;
}

void DeviceHandler::setDevice(DeviceInfo* device) {
    clearMessages();
    currentDevice_ = device;

    // Disconnect and delete old connection
    if(control_) {
        control_->disconnectFromDevice();
        delete control_;
        control_ = nullptr;
    }

    // Create new controller and connect it if device available
    if(currentDevice_) {

        // Make connections
        //! [Connect-Signals-1]
        control_ = QLowEnergyController::createCentral(currentDevice_->getDevice(), this);
        //! [Connect-Signals-1]
        control_->setRemoteAddressType(addressType_);
        //! [Connect-Signals-2]
        connect(control_, &QLowEnergyController::serviceDiscovered, this, &DeviceHandler::serviceDiscovered);
        connect(control_, &QLowEnergyController::discoveryFinished, this, &DeviceHandler::serviceScanDone);

        connect(control_, &QLowEnergyController::errorOccurred, this, [this](QLowEnergyController::Error error) {
            Q_UNUSED(error);
            setError("Cannot connect to remote device.");
            setIcon(IconError);
        });
        connect(control_, &QLowEnergyController::connected, this, [this]() {
            setInfo("Controller connected. Search services...");
            setIcon(IconProgress);
            control_->discoverServices();
        });
        connect(control_, &QLowEnergyController::disconnected, this, [this]() {
            setError("LowEnergy controller disconnected");
            setIcon(IconError);
        });

        // Connect
        control_->connectToDevice();
        //! [Connect-Signals-2]
    }
}

void DeviceHandler::startMeasurement() {
    if(alive()) {
        runing_ = true;
        emit runingChanged();
    }
}

void DeviceHandler::stopMeasurement() {
    runing_ = false;
    emit runingChanged();
}

//! [Filter HeartRate service 1]
void DeviceHandler::serviceDiscovered(const QBluetoothUuid& gatt) {
    if(gatt == FileService) {
        setInfo("File service discovered. Waiting for service scan to be done...");
        setIcon(IconProgress);
        foundHeartRateService_ = true;
    }
}
//! [Filter HeartRate service 1]

void DeviceHandler::serviceScanDone() {
    setInfo("Service scan done.");
    setIcon(IconBluetooth);

    // Delete old service if available
    if(service_) {
        delete service_;
        service_ = nullptr;
    }

    //! [Filter HeartRate service 2]
    // If heartRateService found, create new service
    if(foundHeartRateService_)
        service_ = control_->createServiceObject(FileService, this);

    if(service_) {
        connect(service_, &QLowEnergyService::stateChanged, this, &DeviceHandler::serviceStateChanged);
        connect(service_, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::updateCharacteristicValue);
        connect(service_, &QLowEnergyService::descriptorWritten, this, &DeviceHandler::confirmedDescriptorWrite);

        // connect(service_, &QLowEnergyService::stateChanged, this,
        //     [](QLowEnergyService::ServiceState newState) {
        //         qDebug() << "stateChanged" << newState;
        //     });
        // connect(service_, &QLowEnergyService::characteristicChanged, this,
        //     [](const QLowEnergyCharacteristic& info, const QByteArray& value) {
        //         qDebug() << "characteristicChanged" << info.uuid() << value.toHex().toUpper();
        //     });
        connect(service_, &QLowEnergyService::characteristicRead, this,
            [](const QLowEnergyCharacteristic& info, const QByteArray& value) {
                qDebug() << "characteristicRead" << info.uuid() << value.toHex().toUpper();
            });
        connect(service_, &QLowEnergyService::characteristicWritten, this,
            [](const QLowEnergyCharacteristic& info, const QByteArray& value) {
                qDebug() << "characteristicWritten" << info.uuid() << value.toHex().toUpper();
            });
        connect(service_, &QLowEnergyService::descriptorRead, this,
            [](const QLowEnergyDescriptor& info, const QByteArray& value) {
                qDebug() << "descriptorRead" << info.uuid() << value.toHex().toUpper();
            });
        // connect(service_, &QLowEnergyService::descriptorWritten, this,
        //     [](const QLowEnergyDescriptor& info, const QByteArray& value) {
        //         qDebug() << "descriptorWritten" << info.uuid() << value.toHex().toUpper();
        //     });
        connect(service_, &QLowEnergyService::errorOccurred, this,
            [](QLowEnergyService::ServiceError error) {
                qDebug() << "errorOccurred" << error;
            });

        service_->discoverDetails();
    } else {
        setError("File Service not found.");
        setIcon(IconError);
    }
    //! [Filter HeartRate service 2]
}

// Service functions
//! [Find HRM characteristic]
void DeviceHandler::serviceStateChanged(QLowEnergyService::ServiceState s) {
    switch(s) {
    case QLowEnergyService::RemoteServiceDiscovering:
        setInfo(tr("Discovering services..."));
        setIcon(IconProgress);
        break;
    case QLowEnergyService::RemoteServiceDiscovered: {
        setInfo(tr("Service discovered."));
        setIcon(IconBluetooth);
        auto setup = [this](std::optional<QLowEnergyCharacteristic*> notificationChar, QLowEnergyDescriptor& notificationDesc, const QBluetoothUuid& uuid) {
            const QLowEnergyCharacteristic hrChar = service_->characteristic(uuid);
            qWarning() << hrChar.uuid() << hrChar.value() << hrChar.clientCharacteristicConfiguration().type();
            if(!hrChar.isValid()) {
                setError("Uuid Data not found.");
                setIcon(IconError);
                return false;
            }
            if(notificationChar) *notificationChar.value() = hrChar;
            notificationDesc = hrChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if(notificationDesc.isValid()) {
                service_->writeDescriptor(notificationDesc, QByteArray::fromHex("0100")); // enable notification
                return false;
            }
            return true;
        };
        setup(&characteristicControl_, notificationDescControl_, FileControlChar);
        setup(std::nullopt, notificationDescReceive_, FileReceiveChar);

        break;
    }
    default:
        // nothing for now
        break;
    }

    emit aliveChanged();
}
//! [Find HRM characteristic]

//! [Reading value]
void DeviceHandler::updateCharacteristicValue(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    qDebug() << sender() << c.uuid() << value.toHex().toUpper();
#if 0
    // ignore any other characteristic change -> shouldn't really happen though
    if(c.uuid() != QBluetoothUuid(QBluetoothUuid::CharacteristicType::HeartRateMeasurement))
        return;

    auto data = reinterpret_cast<const quint8*>(value.constData());
    quint8 flags = *data;
#endif
}

//! [Reading value]
void DeviceHandler::confirmedDescriptorWrite(const QLowEnergyDescriptor& d, const QByteArray& value) {
    qDebug() << sender() << d.uuid() << value.toHex().toUpper();
#if 1
    if(d.isValid() && /*d == notificationDescControl_ &&*/ value == QByteArray::fromHex("0000")) {
        // disabled notifications -> assume disconnect intent
        control_->disconnectFromDevice();
        delete service_;
        service_ = nullptr;
    }
#endif
}

void DeviceHandler::disconnectService() {
    foundHeartRateService_ = false;

    // disable notifications
    auto disconnect = [this](const QLowEnergyDescriptor& notificationDesc_) {
        if(notificationDesc_.isValid() && service_ && notificationDesc_.value() == QByteArray::fromHex("0100")) {
            service_->writeDescriptor(notificationDesc_, QByteArray::fromHex("0000"));
        } else {
            // if(control_)
            //     control_->disconnectFromDevice();
            // delete service_;
            // service_ = nullptr;
        }
    };

    disconnect(notificationDescControl_);
    disconnect(notificationDescReceive_);

    if(control_)
        control_->disconnectFromDevice();

    delete service_;
    service_ = nullptr;
}

bool DeviceHandler::runing() const { return runing_; }

bool DeviceHandler::alive() const {
    if(service_)
        return service_->state() == QLowEnergyService::RemoteServiceDiscovered;

    return false;
}

bool DeviceHandler::write(const QByteArray& hex) {
    if(!alive() /*|| !notificationDescControl_.isValid()*/) return false;
    service_->writeCharacteristic(characteristicControl_, QByteArray::fromHex(hex));
    return true;
}
