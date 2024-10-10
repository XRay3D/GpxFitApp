import QtCharts
import QtLocation
import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material
import QtQuick.Effects

ApplicationWindow {
    id: app
    width: 720
    height: 720
    visible: true
    title: qsTr("Hello World")

    // Material.theme: Material.Dark
    // Material.accent: Material.Purple
    // Material.accent : color
    // Material.background : color
    // Material.elevation : int
    // Material.foreground : color
    // Material.primary : color
    // Material.roundedScale: Material.NotRoundedSquare
    // ConstantDescription
    // Material.NotRoundedSquare corners
    // Material.ExtraSmallScaleExtra small rounded corners
    // Material.SmallScaleSmall rounded corners
    // Material.MediumScaleMedium rounded corners
    // Material.LargeScaleLarge rounded corners
    // Material.ExtraLargeScaleExtra large rounded corners
    // Material.FullScaleFully rounded corners

    // Material.containerStyle : enumeration
    required property ConnectionHandler connectionHandler
    required property DeviceFinder deviceFinder
    required property DeviceHandler deviceHandler

    header: Label {
        height: footer.height
        width: parent.width
        text: "FitApp"
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    footer: TabBar {
        id: tabBar
        // height: parent.height * 0.1
        width: parent.width
        currentIndex: swipeView.currentIndex
        onCurrentIndexChanged: swipeView.currentIndex = currentIndex

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
        Page1 {
            id: page1
        }
        Page2 {
            id: page2
        }
        Page3 {
            id: page3
        }
        Page4 {
            id: page4
        }
        Page5 {
            id: page5
        }
    }

    readonly property int sc: 1

    Connections {
        target: app
        function onSceneGraphInitialized() {
            swipeView.currentIndex = 2
            page3.dialogOpen()
        }
    }
}
