import QtQuick 2.15

Item {
    id: root
    property bool positionLightOn: false
    property bool lowBeamOn: false
    property bool highBeamOn: false
    property bool fogLightOn: false
    property bool leftSignalVisible: false
    property bool rightSignalVisible: false
    property bool seatbeltOn: false
    property bool parkingBrakeOn: false
    property int warningLevel: 0

    signal lightClicked()
    signal highBeamClicked()
    signal fogClicked()
    signal leftClicked()
    signal rightClicked()
    signal warningClicked()

    Image {
        width: 29; height: 29
        source: leftSignalVisible ? "qrc:/assets/icons/arrow-left.svg"
                                  : "qrc:/assets/icons/arrow-left-bold-off.svg"
        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
        fillMode: Image.PreserveAspectFit
        MouseArea { anchors.fill: parent; onClicked: root.leftClicked() }
    }

    Row {
        anchors.left: parent.left; anchors.leftMargin: 150
        anchors.verticalCenter: parent.verticalCenter; spacing: 20

        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: positionLightOn ? "qrc:/assets/icons/car-parking-lights.svg"
                                    : "qrc:/assets/icons/car-parking-lights-off.svg"
            MouseArea { anchors.fill: parent; onClicked: root.lightClicked() }
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: lowBeamOn ? "qrc:/assets/icons/car-light-dimmed.svg"
                              : "qrc:/assets/icons/car-light-dimmed-off.svg"
            MouseArea { anchors.fill: parent; onClicked: root.lightClicked() }
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: highBeamOn ? "qrc:/assets/icons/car-light-high.svg"
                               : "qrc:/assets/icons/car-light-high-off.svg"
            MouseArea { anchors.fill: parent; onClicked: root.highBeamClicked() }
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: fogLightOn ? "qrc:/assets/icons/car-light-fog.svg"
                               : "qrc:/assets/icons/car-light-fog-off.svg"
            MouseArea { anchors.fill: parent; onClicked: root.fogClicked() }
        }
    }

    Row {
        anchors.right: parent.right; anchors.rightMargin: 150
        anchors.verticalCenter: parent.verticalCenter; spacing: 18

        Image {
            width: 30; height: 30; fillMode: Image.PreserveAspectFit
            source: seatbeltOn ? "qrc:/assets/icons/seatbelt.svg"
                               : "qrc:/assets/icons/seatbelt-off.svg"
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: parkingBrakeOn ? "qrc:/assets/icons/car-brake-parking.svg"
                                   : "qrc:/assets/icons/car-brake-parking-off.svg"
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: "qrc:/assets/icons/car-brake-abs-off.svg"
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: "qrc:/assets/icons/car-traction-control-off.svg"
        }
        Image {
            width: 31; height: 31; fillMode: Image.PreserveAspectFit
            source: warningLevel === 2 ? "qrc:/assets/icons/alert-red.svg"
                  : warningLevel === 1 ? "qrc:/assets/icons/alert-amber.svg"
                                       : "qrc:/assets/icons/alert-outline-off.svg"
            MouseArea { anchors.fill: parent; onClicked: root.warningClicked() }
        }
    }

    Image {
        width: 29; height: 29
        source: rightSignalVisible ? "qrc:/assets/icons/arrow-right.svg"
                                   : "qrc:/assets/icons/arrow-right-bold-off.svg"
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
        fillMode: Image.PreserveAspectFit
        MouseArea { anchors.fill: parent; onClicked: root.rightClicked() }
    }
}
