#pragma once

#include <QAbstractTableModel>
#include <QObject>
#include <QQmlEngine>
#include <QtPositioning>

class Model : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    using QAbstractTableModel::QAbstractTableModel;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Q_INVOKABLE QList<QGeoCoordinate> route() const;
};

class Fit /*final*/ : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)

public:
    explicit Fit(QObject *parent = nullptr);
    ~Fit() override = default;

    QString text() const { return text_; }
    void setText(const QString &newText)
    {
        if (text_ == newText)
            return;
        text_ = newText;
        emit textChanged();
    }

    bool loadFile(const QString &filePath);

signals:
    void textChanged();

private:
    QString text_{"__FUNCTION__"};
};
