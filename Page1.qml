import QtCharts
import QtLocation
import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {

    Plugin {
        id: mapPlugin
        name: "osm"
        locales: 'ru_RU'
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Map {
            id: map
            // anchors.fill: parent
            Layout.fillHeight: true
            Layout.fillWidth: true
            plugin: mapPlugin
            center: Model.route()[0]
            //QtPositioning.coordinate(56.858754, 35.917349) // Тверь
            zoomLevel: 14
            property geoCoordinate startCentroid

            PinchHandler {
                id: pinch
                target: null
                onActiveChanged: if (active) {
                                     map.startCentroid = map.toCoordinate(
                                                 pinch.centroid.position, false)
                                 }
                onScaleChanged: delta => {
                                    map.zoomLevel += Math.log2(delta)
                                    map.alignCoordinateToPoint(
                                        map.startCentroid,
                                        pinch.centroid.position)
                                }
                onRotationChanged: delta => {
                                       map.bearing -= delta
                                       map.alignCoordinateToPoint(
                                           map.startCentroid,
                                           pinch.centroid.position)
                                   }
                grabPermissions: PointerHandler.TakeOverForbidden
            }
            WheelHandler {
                id: wheel
                // workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
                // Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
                // and we don't yet distinguish mice and trackpads on Wayland either
                acceptedDevices: Qt.platform.pluginName === "cocoa"
                                 || Qt.platform.pluginName
                                 === "wayland" ? PointerDevice.Mouse
                                                 | PointerDevice.TouchPad : PointerDevice.Mouse
                rotationScale: 1 / 120
                property: "zoomLevel"
            }
            DragHandler {
                id: drag
                target: null
                onTranslationChanged: delta => map.pan(-delta.x, -delta.y)
            }
            Shortcut {
                enabled: map.zoomLevel < map.maximumZoomLevel
                sequence: StandardKey.ZoomIn
                onActivated: map.zoomLevel = Math.round(map.zoomLevel + 1)
            }
            Shortcut {
                enabled: map.zoomLevel > map.minimumZoomLevel
                sequence: StandardKey.ZoomOut
                onActivated: map.zoomLevel = Math.round(map.zoomLevel - 1)
            }
            MapPolyline {
                line.width: 3
                line.color: 'green'
                path: Model.route()
            }
        }
        Flickable {
            id: flickable
            // anchors.fill: parent
            Layout.fillHeight: true
            Layout.fillWidth: true
            contentWidth: column.width
            contentHeight: 200 * (Model.columnCount() - 1)
            clip: true

            Column {
                id: column
                anchors.fill: parent

                // Text {
                //     id: text
                //     text: fit.text
                // }
                Repeater {
                    id: repeater
                    model: Model.columnCount() - 1
                    ChartView {
                        id: chartView
                        title: lineSeries.name
                        // anchors.fill: parent
                        width: flickable.width
                        height: 200
                        antialiasing: true
                        legend.visible: false

                        margins.bottom: 0
                        margins.left: 0
                        margins.right: 0
                        margins.top: 0

                        DateTimeAxis {
                            id: valueAxis
                            // min: 2000
                            // max: 2011
                            // tickCount: 12
                            // labelFormat: "%.0f"
                        }
                        SplineSeries {
                            id: lineSeries
                            name: Model.headerData(index, Qt.Horizontal,
                                                   Qt.DisplayRole)
                            axisX: valueAxis
                            capStyle: Qt.RoundCap
                            VXYModelMapper {
                                // firstRow: 0
                                model: Model
                                // rowCount: 0
                                series: lineSeries
                                xColumn: repeater.model
                                yColumn: index
                            }
                        }
                    }
                }
            }
        }
    }

    // FileDialog {
    //     id: myFileDialog
    //     title: qsTr("Select a File")
    //     selectExisting: true
    //     selectFolder: false
    //     // to make it simpler in this example we only deal with the first selected file
    //     // HINT: on IOS there always can only be selected ONE file
    //     // to enable multi files you have to create a FileDialog by yourself === some work to get a great UX
    //     selectMultiple: true
    //     folder: Qt.platform.os === "ios"? shortcuts.pictures:""
    //     onAccepted: {
    //         if(fileUrls.length) {
    //             console.log("we selected: ", fileUrls[0])
    //             // hint: on IOS the fileUrl must be converted to local file
    //             // to be understood by CPP see shareUtils.verifyFileUrl
    //             if(shareUtils.verifyFileUrl(fileUrls[0])) {
    //                 console.log("YEP: the file exists")
    //                 popup.labelText = qsTr("The File exists and can be accessed\n%1").arg(fileUrls[0])
    //                 popup.open()
    //                 // it's up to you to copy the file, display image, etc
    //                 // here we only check if File can be accessed or not
    //                 return
    //             } else {
    //                 if(Qt.platform.os === "ios") {
    //                     popup.labelText = qsTr("The File CANNOT be accessed.\n%1").arg(fileUrls[0])
    //                 } else {
    //                     // Attention with Spaces or Umlauts in FileName if 5.15.14
    //                     // see https://bugreports.qt.io/browse/QTBUG-114435
    //                     // and workaround for SPACES: https://bugreports.qt.io/browse/QTBUG-112663
    //                     popup.labelText = qsTr("The File CANNOT be accessed.\nFile Names with spaces or Umlauts please wait for Qt 5.15.15\n%1").arg(fileUrls[0])
    //                 }
    //                 popup.open()
    //             }
    //         } else {
    //             console.log("no file selected")
    //         }
    //     }
    // } // myFileDialog
}
