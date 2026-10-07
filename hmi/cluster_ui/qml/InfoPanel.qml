import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property int currentPage: 0
    property bool menuActive: false
    property int dteKm: 0
    property int soc: 0
    property real tripKm: 0
    property int tripMinutes: 0
    property int warningLevel: 0
    property string warningText: ""
    property int brightness: 100
    signal pageSelected(int index)
    readonly property var icons: ["directions_car", "bolt", "flag", "warning", "brightness_6"]
    readonly property var titles: ["VEHICLE", "ENERGY", "TRIP", "WARNINGS", "DISPLAY"]

    Column {
        anchors.fill: parent
        spacing: 10
        Column {
            width: parent.width; spacing: -2
            Label {
                text: root.dteKm; color: "#f5f7f8"
                font.pixelSize: 43; font.weight: Font.Light
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label { text: "km"; color: "#9ba2a7"; font.pixelSize: 15; anchors.horizontalCenter: parent.horizontalCenter }
        }
        Rectangle { width: parent.width; height: 1; color: "#3b4247" }
        Row {
            width: parent.width; height: 38; spacing: 6
            Repeater {
                model: root.icons
                Item {
                    width: (parent.width - 24) / 5; height: 38
                    Image {
                        width: 20; height: 20
                        anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top
                        source: "qrc:/assets/icons/menu-" + modelData
                                + (root.menuActive && index === root.currentPage ? "-active.svg" : ".svg")
                        fillMode: Image.PreserveAspectFit
                    }
                    Rectangle {
                        width: 28; height: 2
                        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
                        color: "#20c9d8"; opacity: root.menuActive && index === root.currentPage ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 160 } }
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.pageSelected(index) }
                }
            }
        }
        Label {
            visible: root.menuActive
            text: root.titles[root.currentPage]
            color: root.menuActive ? "#20c9d8" : "#92999e"
            font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.5
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Loader {
            width: parent.width; height: 120
            sourceComponent: !root.menuActive ? tripPage
                             : currentPage === 0 ? vehiclePage
                             : currentPage === 1 ? energyPage
                             : currentPage === 2 ? tripPage
                             : currentPage === 3 ? warningPage : displayPage
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    Component {
        id: vehiclePage
        Column {
            spacing: 11; anchors.horizontalCenter: parent.horizontalCenter
            InfoLine { iconSource: "qrc:/assets/icons/menu-bolt.svg"; label: "POWER"; value: backend.motorOutput + "%" }
            InfoLine { iconSource: "qrc:/assets/icons/menu-directions_car.svg"; label: "MODE"; value: backend.driveModeName }
            InfoLine { iconSource: "qrc:/assets/icons/regen.svg"; label: "REGEN"; value: "LEVEL " + backend.regenLevel }
        }
    }
    Component {
        id: energyPage
        Column {
            spacing: 11; anchors.horizontalCenter: parent.horizontalCenter
            InfoLine { iconSource: "qrc:/assets/icons/battery.svg"; label: "SOC"; value: root.soc + "%" }
            InfoLine { iconSource: "qrc:/assets/icons/menu-bolt.svg"; label: "RANGE"; value: root.dteKm + " km" }
            InfoLine { iconSource: "qrc:/assets/icons/menu-bolt.svg"; label: "AVERAGE"; value: "15.8 kWh/100km" }
        }
    }
    Component {
        id: tripPage
        Column {
            spacing: 11; anchors.horizontalCenter: parent.horizontalCenter
            InfoLine { iconSource: "qrc:/assets/icons/menu-flag.svg"; label: "TRIP A"; value: root.tripKm.toFixed(1) + " km" }
            InfoLine { iconSource: "qrc:/assets/icons/clock.svg"; label: "DRIVE"; value: root.tripMinutes + " min" }
            InfoLine {
                iconSource: "qrc:/assets/speedometer.svg"; label: "AVERAGE"
                value: Math.round(root.tripKm * 60 / Math.max(1, root.tripMinutes)) + " km/h"
            }
        }
    }
    Component {
        id: warningPage
        Column {
            spacing: 8; anchors.horizontalCenter: parent.horizontalCenter
            Image {
                width: 30; height: 30; anchors.horizontalCenter: parent.horizontalCenter
                source: root.warningLevel === 2 ? "qrc:/assets/icons/alert-red.svg"
                      : root.warningLevel === 1 ? "qrc:/assets/icons/alert-amber.svg"
                                               : "qrc:/assets/icons/menu-warning-active.svg"
                fillMode: Image.PreserveAspectFit
            }
            Label {
                width: 270
                text: root.warningLevel === 0 ? "NO ACTIVE DTC" : root.warningText
                color: "#f5f7f8"; font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
            }
            Label {
                text: root.warningLevel === 0 ? "Vehicle warnings clear" : "Press D to acknowledge"
                color: "#8b9298"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
    Component {
        id: displayPage
        Column {
            spacing: 9; anchors.horizontalCenter: parent.horizontalCenter
            InfoLine { iconSource: "qrc:/assets/icons/menu-brightness_6.svg"; label: "BRIGHTNESS"; value: root.brightness + "%" }
            Row {
                spacing: 5; anchors.horizontalCenter: parent.horizontalCenter
                Repeater {
                    model: 5
                    Rectangle {
                        width: 31; height: 5; radius: 2
                        color: index < Math.ceil(root.brightness / 20) ? "#20c9d8" : "#42494e"
                    }
                }
            }
            InfoLine { iconSource: "qrc:/assets/icons/menu-brightness_6.svg"; label: "THEME"; value: "AUTO" }
            InfoLine { iconSource: "qrc:/assets/speedometer.svg"; label: "UNITS"; value: "km/h" }
        }
    }
}
