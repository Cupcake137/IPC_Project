import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    height: 32
    property date now: new Date()
    Timer { interval: 1000; running: true; repeat: true; onTriggered: root.now = new Date() }
    Row {
        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 9
        Image { width: 22; height: 22; source: "qrc:/assets/icons/weather.svg"; fillMode: Image.PreserveAspectFit }
        Label { text: "27°C"; color: "#f5f7f8"; font.pixelSize: 18; font.bold: true }
    }
    Row {
        anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: parent.verticalCenter; spacing: 8
        Image { width: 20; height: 20; source: "qrc:/assets/icons/clock.svg"; fillMode: Image.PreserveAspectFit }
        Label { text: Qt.formatDateTime(root.now, "hh:mm"); color: "#f5f7f8"; font.pixelSize: 19; font.bold: true }
    }
    Row {
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 9
        Image { width: 20; height: 20; source: "qrc:/assets/icons/calendar.svg"; fillMode: Image.PreserveAspectFit }
        Label { text: Qt.formatDateTime(root.now, "dd/MM/yyyy"); color: "#f5f7f8"; font.pixelSize: 18; font.bold: true }
    }
}
