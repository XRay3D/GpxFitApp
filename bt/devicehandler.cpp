// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "devicehandler.h"
#include "deviceinfo.h"

#include <QtCore/qendian.h>
#include <QtCore/qrandom.h>

#include <QApplication>
#include <QDir>

// 8ce5cc030a4d11e9ab14d663bd873d93
//                               "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
const QBluetoothUuid FileService{"{8ce5cc01-0a4d-11e9-ab14-d663bd873d93}"};
const QBluetoothUuid FileControlChar{"{8ce5cc02-0a4d-11e9-ab14-d663bd873d93}"};
const QBluetoothUuid FileReceiveChar{"{8ce5cc03-0a4d-11e9-ab14-d663bd873d93}"};

enum Cmd : uint16_t {
    GetFits = 0x4940,
    GetFit = 0x4A40,
};

struct [[gnu::packed]] ReqFits {
    const Cmd cmd{GetFits};
    uint32_t date{};
    explicit ReqFits(const QDateTime& date)
        : date{static_cast<uint32_t>(date.toSecsSinceEpoch())} { }
    operator QByteArray() const {
        QByteArray ret{sizeof(ReqFits), '\0'};
        memcpy(ret.data(), this, sizeof(ReqFits));
        return ret;
    }
};

struct [[gnu::packed]] RcievedFits {
    Cmd cmd{GetFits};
    uint8_t _{};
    uint8_t count{};
    uint8_t hasMore{};
    QList<QDateTime> fits;
    enum {
        Size = 5
    };
    explicit RcievedFits(const QByteArray& data) {
        memcpy(this, data.data(), Size);
        fits.reserve(count);
        const char* ptr = data.data() + Size;
        qDebug() << count << hasMore << Size;
        for(uint16_t i{}; i < count; ++i) {
            uint32_t date{};
            memcpy(&date, ptr, sizeof(date));
            ptr += 4;
            fits << QDateTime::fromSecsSinceEpoch(date);
            qDebug() << i << fits.back();
        }
    }
    operator QList<QDateTime>() const { return fits; }
};

struct [[gnu::packed]] ReqFit {
    const Cmd cmd{GetFit};
    uint32_t date{};
    explicit ReqFit(const QDateTime& date)
        : date{static_cast<uint32_t>(date.toSecsSinceEpoch())} { }
    operator QByteArray() const {
        QByteArray ret{sizeof(ReqFit), '\0'};
        memcpy(ret.data(), this, sizeof(ReqFit));
        return ret;
    }
};

struct [[gnu::packed]] RcieveFit {
    uint32_t date{};
    uint32_t dataLeft{};
    uint16_t kuskov{};
    uint16_t kusok{};
    uint8_t dataSize{};
    QByteArray data;
    enum {
        Size = 13
    };
    explicit RcieveFit(const QByteArray& data)
        : data{data.mid(Size)} {
        memcpy(this, data.data(), Size);
    }
};

DeviceHandler::DeviceHandler(QObject* parent)
    : BluetoothBaseClass{parent} {

    if(QDir dir{QApplication::applicationDirPath()}; dir.exists()) {                                       // Поиск всех файлов в папке "plugins"
        for(auto listFiles = dir.entryList(QStringList("*.fit"), QDir::Files); const auto& str: listFiles) // Проход по всем файлам
            fitFiles_.push_back(QDateTime::fromString(str.mid(0, str.size() - 4), "dd.MM.yyyy hh:mm:ss"));
        std::ranges::sort(fitFiles_);
        qWarning() << fitFiles_;
    }
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

// void DeviceHandler::startMeasurement() {
//     if(alive()) {
//         runing_ = true;
//         emit runingChanged();
//     }
// }

// void DeviceHandler::stopMeasurement() {
//     runing_ = false;
//     emit runingChanged();
// }

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
    if(FileControlChar == c.uuid()) {
        if(value.size() < 2) return;
        Cmd cmd{};
        memcpy(&cmd, value.data(), sizeof(Cmd));
        switch(cmd) {
        case GetFits: {
            RcievedFits fits{value};
            if(fits.count) {
                fits_.append(fits);
                fitsChanged();
            }
            if((runing_ = fits.hasMore))
                write(ReqFits{fits_.back()});
        } break;
        case GetFit: {
            if(value.size() == 3 && value.back() == 0) {
                file.open(QFile::WriteOnly);
            }
        } break;
        default: break;
        }
    } else if(FileReceiveChar == c.uuid()) {
        RcieveFit fit{value};
        file.write(fit.data);
        setKuskov(fit.kuskov);
        setKusok(fit.kusok);
        if(fit.dataLeft == 0 && fit.kuskov == fit.kusok) {
            fitFiles_.push_back(QDateTime::fromSecsSinceEpoch(fit.date));
            file.close();
            loop.exit();
        }
    }
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

void DeviceHandler::getFitsAfterDate(const QDateTime& date) {
    if(runing_) return;
    fits_.clear();
    fitsChanged();
    write(ReqFits{date});
}

void DeviceHandler::getLast() {
    if(runing_) return;
    auto date = fitFiles_.last().addSecs(1);
    fits_.clear();
    fitsChanged();
    write(ReqFits{date});
}

bool DeviceHandler::runing() const { return runing_; }

bool DeviceHandler::alive() const {
    if(service_)
        return service_->state() == QLowEnergyService::RemoteServiceDiscovered;

    return false;
}

bool DeviceHandler::write(const QByteArray& data) {
    if(!alive() /*|| !notificationDescControl_.isValid()*/) return false;
    service_->writeCharacteristic(characteristicControl_, data);
    return runing_ = true;
}

void DeviceHandler::downloadAllFits() {
    for(int i{}; auto&& fit: fits_) {
        setCurrentFile(++i);
        file.setFileName(fit.toString("dd.MM.yyyy hh:mm:ss") + ".fit");
        write(ReqFit{fit});
        loop.exec();
    }
    setCurrentFile(0);
}
