// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BLUETOOTHBASECLASS_H
#define BLUETOOTHBASECLASS_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class BluetoothBaseClass : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString error READ error WRITE setError NOTIFY errorChanged)
    Q_PROPERTY(QString info READ info WRITE setInfo NOTIFY infoChanged)
    Q_PROPERTY(IconType icon READ icon WRITE setIcon NOTIFY iconChanged)

    QML_ELEMENT
    QML_UNCREATABLE("BluetoothBaseClass is not intended to be created directly")

public:
    enum IconType : int {
        IconNone,
        IconBluetooth,
        IconError,
        IconProgress,
        IconSearch
    };
    Q_ENUM(IconType)

    explicit BluetoothBaseClass(QObject *parent = nullptr);

    QString error() const;
    void setError(const QString& error);

    QString info() const;
    void setInfo(const QString& info);

    IconType icon() const;
    void setIcon(IconType icon);

    void clearMessages();

signals:
    void errorChanged();
    void infoChanged();
    void iconChanged();

private:
    QString error_;
    QString info_;
    IconType icon_ = IconNone;
};

#endif // BLUETOOTHBASECLASS_H
