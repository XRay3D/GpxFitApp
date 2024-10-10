import QtCharts
import QtLocation
import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {

    // Rectangle {
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
