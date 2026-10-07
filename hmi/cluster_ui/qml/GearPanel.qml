import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property string gear: "P"
    property color modeColor: "#20c9d8"
    Row {
        id: gears
        anchors.horizontalCenter: parent.horizontalCenter; spacing: 20
        Repeater {
            model: ["P", "R", "N", "D"]
            Label {
                text: modelData
                color: root.gear === modelData ? root.modeColor : "#676d71"
                font.pixelSize: 22; font.bold: root.gear === modelData
                Behavior on color { ColorAnimation { duration: 220 } }
            }
        }
    }
    Image {
        id: car
        width: 150; height: 215
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: gears.bottom; anchors.topMargin: 18
        source: "qrc:/assets/compact-city-ev.png"
        fillMode: Image.PreserveAspectFit; smooth: true
    }
    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: car.bottom; anchors.topMargin: 5
        text: gear === "P" ? "PARKED" : gear === "R" ? "REVERSE" : gear === "N" ? "NEUTRAL" : "DRIVE"
        color: gear === "P" ? "#a1a7ac" : modeColor
        font.pixelSize: 14; font.bold: true
        Behavior on color { ColorAnimation { duration: 220 } }
    }
}
