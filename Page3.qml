import QtCharts
import QtLocation
import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: bt

    function dialogOpen() {
        // dialogProgress.open()
        findDevDialog.open()
        deviceFinder.startSearch()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        Button {
            Layout.fillWidth: true
            onClicked: {
                deviceHandler.getFitsAfterDate(Date.fromLocaleString(
                                                   Qt.locale(),
                                                   '2013.09.17 00:00:00',
                                                   'yyyy.MM.dd hh:mm:ss'))
            }
            text: 'getFitsAfterDate'
        }
        Button {
            Layout.fillWidth: true
            onClicked: deviceHandler.getLast()
            text: 'getLast'
        }
        Button {
            Layout.fillWidth: true
            onClicked: {
                dialogProgress.open()
                deviceHandler.downloadAllFits()
            }
            text: 'getAll'
        }
        ListView {
            id: listView
            Layout.fillHeight: true
            Layout.fillWidth: true
            model: deviceHandler.fits
            clip: true
            delegate: Button {
                width: parent.width
                text: modelData.toLocaleString(Qt.locale(),
                                               'dd.MM.yyyy hh:mm:ss')
            }
        }
    }
    Dialog {
        id: dialogProgress

        // Material.roundedScale: Material.NotRoundedSquare
        property string caption: 'Dialog'

        readonly property int offset: 50

        // Material.roundedScale: Material.MediumScaleMedium
        x: offset
        y: offset
        width: parent.width - offset * 2
        height: parent.height - offset * 2

        standardButtons: Dialog.Close

        // contentItem: Rectangle {
        //     id: contentItemRectangle
        //     anchors.fill: parent
        //     anchors.topMargin: header.height
        //     color: 'red'
        //     radius: 30 * sc
        // }
        header: Label {
            text: deviceHandler.fits[deviceHandler.currentFile - 1]
            color: '#28343e'
            font.pixelSize: 36 * sc
            height: 73 * sc
        }
        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true // text:  deviceHandler.fits[deviceHandler.]
                text: listView.itemAtIndex(deviceHandler.currentFile - 1).text
            }
            ProgressBar {
                from: 1
                to: listView.count
                value: deviceHandler.currentFile
                Layout.fillWidth: true
            }
            ProgressBar {
                from: 1
                to: deviceHandler.kuskov
                value: deviceHandler.kusok
                Layout.fillWidth: true
            }
        }
        Connections {
            target: deviceHandler
            function onCurrentFile() {
                if (!deviceHandler.currentFile)
                    dialogProgress.close()
                else
                    dialogProgress.open()
            }
        }

        onClosed: deviceFinder.stopSearch()
    }
    Dialog {
        id: findDevDialog

        // Material.roundedScale: Material.NotRoundedSquare
        property string caption: 'Dialog'

        readonly property int offset: 50

        // Material.roundedScale: Material.MediumScaleMedium
        x: offset
        y: offset
        width: parent.width - offset * 2
        height: parent.height - offset * 2

        standardButtons: Dialog.Close

        // contentItem: Rectangle {
        //     id: contentItemRectangle
        //     anchors.fill: parent
        //     anchors.topMargin: header.height
        //     color: 'red'
        //     radius: 30 * sc
        // }
        header: Label {
            text: findDevDialog.caption
            color: '#28343e'
            font.pixelSize: 36 * sc
            height: 73 * sc
        }

        ListView {
            anchors.fill: parent
            model: deviceFinder.devices
            delegate: Button {
                width: parent.width
                text: modelData.deviceName
                onClicked: {
                    console.warn('address', modelData.deviceAddress)
                    deviceFinder.stopSearch()
                    deviceFinder.connectToService(modelData.deviceAddress)
                }
            }
        }

        Connections {
            target: deviceHandler
            function onAliveChanged() {
                if (deviceHandler.alive)
                    findDevDialog.close()
            }
        }

        onClosed: deviceFinder.stopSearch()
    }
}
