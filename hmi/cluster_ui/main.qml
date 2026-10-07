import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "qml"

ApplicationWindow {
    id: window
    width: 1120; height: 610
    minimumWidth: 820; minimumHeight: 460
    visible: true
    color: "#090c0f"
    title: "IPC Electric Cluster"

    property alias currentPage: menu.currentPage
    property alias menuActive: menu.active
    property bool blinkOn: true
    readonly property color modeColor: backend.driveMode === 0 ? "#45c96b"
                                         : backend.driveMode === 2 ? "#f28c28" : "#20c9d8"

    MenuController {
        id: menu
        vehicle: backend
        currentPage: startPage
        active: startMenuOpen
    }

    Connections { target: backend; function onUiAction(action) { menu.handleAction(action) } }
    Timer { interval: 500; running: true; repeat: true; onTriggered: blinkOn = !blinkOn }

    Item {
        id: stage
        width: 900; height: 500
        anchors.centerIn: parent
        scale: Math.min(1, (window.width - 36) / width, (window.height - 32) / height)

        Rectangle { anchors.fill: parent; color: "#101417"; radius: 54; border.width: 1; border.color: "#347f82" }
        HeaderBar {
            id: header
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.leftMargin: 38; anchors.rightMargin: 38; anchors.topMargin: 25
        }
        TelltaleBar {
            id: telltales
            anchors.top: header.bottom; anchors.topMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 76; height: 34
            positionLightOn: backend.positionLightOn
            lowBeamOn: backend.lowBeamOn
            highBeamOn: backend.highBeamOn
            fogLightOn: backend.fogLightOn
            leftSignalVisible: (backend.leftSignalOn || backend.hazardOn) && blinkOn
            rightSignalVisible: (backend.rightSignalOn || backend.hazardOn) && blinkOn
            seatbeltOn: backend.gear === "P"
            parkingBrakeOn: backend.gear === "P"
            warningLevel: backend.warningLevel
            onLightClicked: backend.cycleLights()
            onHighBeamClicked: backend.toggleHighBeam()
            onFogClicked: backend.toggleFogLight()
            onLeftClicked: backend.toggleLeftSignal()
            onRightClicked: backend.toggleRightSignal()
            onWarningClicked: menu.openPage(3)
        }
        GearPanel {
            anchors.left: parent.left; anchors.leftMargin: 46
            anchors.top: telltales.bottom; anchors.topMargin: 24
            width: 220; height: 315
            gear: backend.gear; modeColor: window.modeColor
        }
        SpeedGauge {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: telltales.bottom; anchors.topMargin: 22
            width: 280; height: 280
            speed: backend.speed; accentColor: window.modeColor
        }
        InfoPanel {
            anchors.right: parent.right; anchors.rightMargin: 42
            anchors.top: telltales.bottom; anchors.topMargin: 34
            width: 240; height: 285
            currentPage: window.currentPage; menuActive: window.menuActive
            dteKm: backend.dteKm; soc: backend.soc
            tripKm: backend.tripKm; tripMinutes: backend.tripMinutes
            warningLevel: backend.warningLevel; warningText: backend.warningText
            brightness: backend.brightness
            onPageSelected: function(index) { menu.openPage(index) }
        }
        FooterBar {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.leftMargin: 50; anchors.rightMargin: 50; anchors.bottomMargin: 22
            height: 40
            soc: backend.soc; regenLevel: backend.regenLevel
            driveModeName: backend.driveModeName; modeColor: window.modeColor
            odometerKm: backend.odometerKm
        }
    }
    Rectangle {
        anchors.fill: parent; color: "black"
        opacity: (100 - backend.brightness) * 0.0055
        visible: opacity > 0; z: 100
    }
}
