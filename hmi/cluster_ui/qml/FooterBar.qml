import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property int soc: 100
    property int regenLevel: 1
    property string driveModeName: "NORMAL"
    property color modeColor: "#20c9d8"
    property real odometerKm: 0

    Row {
        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 10
        Label { text: "ODO"; color: "#858d92"; font.pixelSize: 15 }
        Label { text: odometerKm.toFixed(1) + " km"; color: "#eef1f2"; font.pixelSize: 16 }
    }
    Row {
        anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: parent.verticalCenter; spacing: 26
        Label {
            text: driveModeName; color: modeColor; font.pixelSize: 17; font.bold: true
            Behavior on color { ColorAnimation { duration: 220 } }
        }
        Row {
            spacing: 8
            Image { width: 21; height: 21; source: "qrc:/assets/icons/regen.svg"; fillMode: Image.PreserveAspectFit }
            Label { text: "REGEN " + regenLevel; color: "#20c9d8"; font.pixelSize: 16; font.bold: true }
        }
    }
    Row {
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 10
        Image { width: 21; height: 21; source: "qrc:/assets/icons/battery.svg"; fillMode: Image.PreserveAspectFit }
        Row {
            spacing: 4; anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model: 5
                Rectangle {
                    width: 23; height: 5; radius: 2
                    color: index < Math.ceil(root.soc / 20)
                           ? (root.soc <= 20 ? "#f2a33a" : "#20c9d8") : "#40474c"
                }
            }
        }
        Label { text: soc + "%"; color: "#f5f7f8"; font.pixelSize: 17; font.bold: true }
    }
}
