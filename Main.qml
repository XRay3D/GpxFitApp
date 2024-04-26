import QtCharts
import QtLocation
import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    width: 720
    height: 720
    visible: true
    title: qsTr("Hello World")

    required property ConnectionHandler connectionHandler
    required property DeviceFinder deviceFinder
    required property DeviceHandler deviceHandler

    header: Label {
        // height: parent.height * 0.1
        width: parent.width
        text: "FitApp"
    }

    footer: TabBar {
        id: tabBar
        // height: parent.height * 0.1
        width: parent.width
        currentIndex: swipeView.currentIndex

        TabButton {
            text: "Page1"
        }
        TabButton {
            text: "Page2"
        }
        TabButton {
            text: "Page3"
        }
        TabButton {
            text: "Page4"
        }
        TabButton {
            text: "Page5"
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        Page1 {}
        Page2 {}
        Page3 {}
        Page4 {}
        Page5 {}
    }
}
