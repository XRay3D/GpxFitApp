// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connectionhandler.h"

#include <QtBluetooth/qtbluetooth-config.h>

#include <QtCore/qsystemdetection.h>

#if QT_CONFIG(permissions)
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>
#endif

ConnectionHandler::ConnectionHandler(QObject* parent)
    : QObject(parent) {
    initLocalDevice();
}

bool ConnectionHandler::alive() const {
#ifdef QT_PLATFORM_UIKIT
    return true;

#else
    return localDevice_ && localDevice_->isValid()
        && localDevice_->hostMode() != QBluetoothLocalDevice::HostPoweredOff;
#endif
}

bool ConnectionHandler::hasPermission() const {
    return hasPermission_;
}

bool ConnectionHandler::requiresAddressType() const {
#if QT_CONFIG(bluez)
    return true;
#else
    return false;
#endif
}

QString ConnectionHandler::name() const {
    return localDevice_ ? localDevice_->name() : QString();
}

QString ConnectionHandler::address() const {
    return localDevice_ ? localDevice_->address().toString() : QString();
}

void ConnectionHandler::hostModeChanged(QBluetoothLocalDevice::HostMode /*mode*/) {
    emit deviceChanged();
}

void ConnectionHandler::initLocalDevice() {
#if QT_CONFIG(permissions)
    QBluetoothPermission permission{};
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch(qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, &ConnectionHandler::initLocalDevice);
        return;
    case Qt::PermissionStatus::Denied:
        return;
    case Qt::PermissionStatus::Granted:
        break; // proceed to initialization
    }
#endif
    localDevice_ = new QBluetoothLocalDevice(this);
    connect(localDevice_, &QBluetoothLocalDevice::hostModeStateChanged,
        this, &ConnectionHandler::hostModeChanged);
    hasPermission_ = true;
    emit deviceChanged();
}
