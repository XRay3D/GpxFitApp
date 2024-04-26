// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bluetoothbaseclass.h"
#include <QDebug>

BluetoothBaseClass::BluetoothBaseClass(QObject* parent)
    : QObject{parent} { }

QString BluetoothBaseClass::error() const { return error_; }

QString BluetoothBaseClass::info() const { return info_; }

void BluetoothBaseClass::setError(const QString& error) {
    if(error_ != error) {
        error_ = error;
        qCritical() << error_;
        emit errorChanged();
    }
}

void BluetoothBaseClass::setInfo(const QString& info) {
    if(info_ != info) {
        info_ = info;
        qInfo() << info_;
        emit infoChanged();
    }
}

BluetoothBaseClass::IconType BluetoothBaseClass::icon() const { return icon_; }

void BluetoothBaseClass::setIcon(IconType icon) {
    if(icon_ != icon) {
        icon_ = icon;
        emit iconChanged();
    }
}

void BluetoothBaseClass::clearMessages() {
    setInfo("");
    setError("");
    setIcon(IconNone);
}
