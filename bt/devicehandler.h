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

#include <QEventLoop>
#include <QFile>

class DeviceInfo;

class DeviceHandler : public BluetoothBaseClass {
    Q_OBJECT

    Q_PROPERTY(bool runing READ runing NOTIFY runingChanged)
    Q_PROPERTY(bool alive READ alive NOTIFY aliveChanged)
    Q_PROPERTY(AddressType addressType READ addressType WRITE setAddressType)
    Q_PROPERTY(QVariant fits READ fits NOTIFY fitsChanged)

    Q_PROPERTY(int currentFile READ currentFile WRITE setCurrentFile NOTIFY currentFileChanged)
    Q_PROPERTY(int kusok READ kusok WRITE setKusok NOTIFY kusokChanged)
    Q_PROPERTY(int kuskov READ kuskov WRITE setKuskov NOTIFY kuskovChanged)

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

    QVariant fits() const { return QVariant::fromValue(fits_); }

    int currentFile() const { return currentFile_; }
    void setCurrentFile(int newCurrentFile) {
        if(currentFile_ == newCurrentFile) return;
        currentFile_ = newCurrentFile;
        emit currentFileChanged();
    }

    int kusok() const { return kusok_; }
    void setKusok(int newKusok) {
        if(kusok_ == newKusok) return;
        kusok_ = newKusok;
        emit kusokChanged();
    }

    int kuskov() const { return kuskov_; }
    void setKuskov(int newKuskov) {
        if(kuskov_ == newKuskov) return;
        kuskov_ = newKuskov;
        emit kuskovChanged();
    }

signals:
    void runingChanged();
    void aliveChanged();
    void fitsChanged();

    void currentFileChanged();
    void kusokChanged();
    void kuskovChanged();

public slots:
    void disconnectService();
    void getFitsAfterDate(const QDateTime& date = {});
    void getLast();
    void downloadAllFits();

private:
    bool write(const QByteArray& data);

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
    QList<QDateTime> fitFiles_;
    QList<QDateTime> fits_;
    QDateTime last;

    QFile file;
    QEventLoop loop;
    int currentFile_;
    int kusok_;
    int kuskov_;
};

#endif // DEVICEHANDLER_H
