import QtCharts
import QtLocation
import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 720
    height: 720
    visible: true
    title: qsTr("Hello World")

    Plugin {
        id: mapPlugin
        name: "osm"
        locales: 'ru_RU'
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
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

        Map {
            id: map
            // anchors.fill: parent
            Layout.fillHeight: true
            Layout.fillWidth: true
            plugin: mapPlugin
            center: QtPositioning.coordinate(56.858754, 35.917349) // Тверь
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

            // RouteQuery {
            //     id: aQuery
            //     waypoints: Model.route()
            // }
            // RouteModel {
            //     id: routeModel
            //     plugin: mapPlugin
            //     query: aQuery
            //     Component.onCompleted: {
            //         var route = Model.route()
            //         for (var i in Model.route())
            //             query.addWaypoint(route[i])
            //         // query.addWaypoint(QtPositioning.coordinate(-25.402340,
            //         //                                            -49.248682))
            //         // query.addWaypoint(QtPositioning.coordinate(-25.392142,
            //         //                                            -49.202556))
            //         // query.addWaypoint(QtPositioning.coordinate(-25.372512,
            //         //                                            -49.227785))
            //         // query.addWaypoint(QtPositioning.coordinate(-25.402340,
            //         //                                            -49.248682))
            //         routeModel.update()
            //     }
            // }
            // MapItemView {
            //     model: routeModel
            //     delegate: MapRoute {
            //         //routeDelegate
            //         route: routeData1
            //         line.color: "red"
            //         line.width: 5
            //         smooth: true
            //     }
            // }
            // // Component {
            // //     id: routeDelegate
            // //     MapRoute {
            // //         route: routeModel
            // //         line.color: 'red'
            // //         line.width: 5
            // //         smooth: true
            // //         opacity: 0.8
            // //     }
            // // }
        }
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            // The background color will show through the cell
            // spacing, and therefore become the grid line color.
            color: Qt.styleHints.appearance === Qt.Light ? palette.mid : palette.midlight

            HorizontalHeaderView {
                id: horizontalHeader
                anchors.left: tableView.left
                anchors.top: parent.top
                clip: true
                syncView: tableView
                textRole: 'display'
            }

            VerticalHeaderView {
                id: verticalHeader
                anchors.left: parent.left
                anchors.top: tableView.top
                clip: true
                syncView: tableView
            }

            TableView {
                id: tableView
                anchors.left: verticalHeader.right
                anchors.top: horizontalHeader.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                clip: true

                columnSpacing: 1
                rowSpacing: 1

                model: Model

                delegate: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 20
                    color: palette.base
                    clip: true
                    // Text {
                    //     id: name
                    //     text: qsTr("text")
                    //     verticalAlignment: Text.AlignVCenter
                    //     horizontalAlignment: Text.AlignHCenter
                    // }
                    Label {
                        anchors.fill: parent
                        text: display
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }


            /*
            Binding {
                target: tableView.flickableItem
                property: "contentY"
                value: (tableView.flickableItem.contentHeight + 16)
                       * vbar.position - (16 * (1 - vbar.position))
            }
            ScrollBar {
                id: vbar
                z: 100
                orientation: Qt.Vertical
                anchors.top: tableView.top
                anchors.left: tableView.right
                anchors.bottom: tableView.bottom
                active: true
                // contentItem: Rectangle {
                //     id: contentItem_rect2
                //     radius: implicitHeight / 2
                //     color: "Red"
                //     width: 10 // This will be overridden by the width of the scrollbar
                //     height: 10 // This will be overridden based on the size of the scrollbar
                // }
                size: (tableView.height) / (tableView.flickableItem.contentItem.height)
                width: 10
            }
*/
        }
    }
}
