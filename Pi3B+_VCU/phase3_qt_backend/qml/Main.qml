import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    minimumWidth: 800
    minimumHeight: 480
    title: qsTr("IPC Automotive Cluster")
    color: "#111417"

    readonly property color foreground: "#f4f7f8"
    readonly property color muted: "#99a4aa"
    readonly property color panel: "#1b2024"
    readonly property color border: "#343b40"
    readonly property color cyan: "#35c4d8"
    readonly property color green: "#5cc489"
    readonly property color amber: "#e5aa52"
    readonly property color danger: "#e45f5f"

    function gearName(value) {
        if (value === 1) return "R"
        if (value === 2) return "N"
        if (value === 3) return "D"
        return "P"
    }

    function stateName(value) {
        if (value === 0) return qsTr("OFF")
        if (value === 1) return qsTr("ACC")
        if (value === 2) return qsTr("READY")
        if (value === 3) return qsTr("CHARGING")
        return qsTr("FAULT")
    }

    header: Rectangle {
        height: 64
        color: window.panel

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 28
            anchors.rightMargin: 28
            spacing: 20

            Label {
                text: qsTr("IPC CLUSTER")
                color: window.foreground
                font.pixelSize: 20
                font.weight: Font.DemiBold
                font.letterSpacing: 0
            }

            Item { Layout.fillWidth: true }

            Label {
                text: vehicleModel.communicationHealthy ? qsTr("ECU LINK") : qsTr("NO ECU DATA")
                color: vehicleModel.communicationHealthy ? window.green : window.danger
                font.pixelSize: 14
                font.letterSpacing: 0
            }

            Label {
                text: simulationMode ? qsTr("SIMULATION") : qsTr("VEHICLE")
                color: simulationMode ? window.amber : window.green
                font.pixelSize: 14
                font.letterSpacing: 0
            }

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: vehicleModel.state === 4 || !vehicleModel.communicationHealthy
                       ? window.danger : window.green
            }

            Label {
                text: stateName(vehicleModel.state)
                color: window.foreground
                font.pixelSize: 14
                font.letterSpacing: 0
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            ColumnLayout {
                Layout.preferredWidth: Math.max(190, window.width * 0.2)
                Layout.fillHeight: true
                spacing: 18

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: window.panel
                    radius: 6
                    border.color: window.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 22

                        Label {
                            text: qsTr("BATTERY")
                            color: window.muted
                            font.pixelSize: 13
                            font.letterSpacing: 0
                        }

                        Item { Layout.fillHeight: true }

                        Label {
                            text: vehicleModel.soc + "%"
                            color: vehicleModel.soc < 15 ? window.danger : window.foreground
                            font.pixelSize: 42
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0
                        }

                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            value: vehicleModel.soc
                        }

                        Label {
                            text: vehicleModel.soc < 15 ? qsTr("LOW CHARGE") : qsTr("AVAILABLE")
                            color: vehicleModel.soc < 15 ? window.danger : window.green
                            font.pixelSize: 13
                            font.letterSpacing: 0
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    color: window.panel
                    radius: 6
                    border.color: window.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 22

                        Label {
                            text: qsTr("PEDAL")
                            color: window.muted
                            font.pixelSize: 13
                            font.letterSpacing: 0
                        }

                        Label {
                            text: vehicleModel.pedal + "%"
                            color: window.foreground
                            font.pixelSize: 32
                            font.letterSpacing: 0
                        }

                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            value: vehicleModel.pedal
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 360

                Canvas {
                    id: speedArc
                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height) * 0.84
                    height: width
                    property real speedValue: vehicleModel.speed
                    onSpeedValueChanged: requestPaint()

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        var cx = width / 2
                        var cy = height / 2
                        var radius = width * 0.41
                        var start = Math.PI * 0.75
                        var span = Math.PI * 1.5

                        ctx.lineWidth = Math.max(10, width * 0.035)
                        ctx.lineCap = "round"
                        ctx.strokeStyle = window.border
                        ctx.beginPath()
                        ctx.arc(cx, cy, radius, start, start + span, false)
                        ctx.stroke()

                        ctx.strokeStyle = vehicleModel.state === 4 ? window.danger : window.cyan
                        ctx.beginPath()
                        ctx.arc(cx, cy, radius, start,
                                start + span * Math.min(speedValue, 120) / 120, false)
                        ctx.stroke()
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 0

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: vehicleModel.speed
                        color: window.foreground
                        font.pixelSize: Math.max(64, Math.min(118, window.height * 0.15))
                        font.weight: Font.Light
                        font.letterSpacing: 0
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("km/h")
                        color: window.muted
                        font.pixelSize: 18
                        font.letterSpacing: 0
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        topPadding: 12
                        text: gearName(vehicleModel.gear)
                        color: vehicleModel.gear === 1 ? window.amber : window.cyan
                        font.pixelSize: 52
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: Math.max(190, window.width * 0.2)
                Layout.fillHeight: true
                spacing: 18

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    color: window.panel
                    radius: 6
                    border.color: window.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 22
                        spacing: 10

                        Label {
                            text: qsTr("DRIVE MODE")
                            color: window.muted
                            font.pixelSize: 13
                            font.letterSpacing: 0
                        }

                        Repeater {
                            model: [qsTr("ECO"), qsTr("NORMAL"), qsTr("SPORT")]

                            Button {
                                Layout.fillWidth: true
                                checkable: true
                                checked: vehicleModel.driveMode === index
                                text: modelData
                                onClicked: vehicleModel.driveMode = index
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: window.panel
                    radius: 6
                    border.color: window.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 22

                        Label {
                            text: qsTr("POWER OUTPUT")
                            color: window.muted
                            font.pixelSize: 13
                            font.letterSpacing: 0
                        }

                        Item { Layout.fillHeight: true }

                        Label {
                            text: Math.round(vehicleModel.authorizedPwm * 100 / 255) + "%"
                            color: window.foreground
                            font.pixelSize: 42
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0
                        }

                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 255
                            value: vehicleModel.authorizedPwm
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: vehicleModel.activeDtc === 0 ? 50 : 72
            color: vehicleModel.activeDtc === 0 ? window.panel : window.danger
            radius: 6
            border.color: vehicleModel.activeDtc === 0 ? window.border : window.danger

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 16

                Label {
                    text: vehicleModel.activeDtc === 0 ? qsTr("SYSTEM OK") : qsTr("DTC")
                    color: window.foreground
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0
                }

                Label {
                    Layout.fillWidth: true
                    text: vehicleModel.activeDtc === 0
                          ? qsTr("No active diagnostic trouble codes")
                          : dtcManager.messageForCode(vehicleModel.activeDtc)
                    color: window.foreground
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    font.letterSpacing: 0
                }

                Label {
                    visible: !simulationMode
                    text: qsTr("RX %1 | DROP %2 | CRC %3")
                          .arg(vehicleModel.receivedFrames)
                          .arg(vehicleModel.droppedFrames)
                          .arg(vehicleModel.checksumErrors)
                    color: window.foreground
                    font.pixelSize: 12
                    font.letterSpacing: 0
                }

                Button {
                    visible: vehicleModel.activeDtc !== 0
                    enabled: dtcManager.rawCode === 0
                             && vehicleModel.gear === 0
                             && vehicleModel.pedal === 0
                             && vehicleModel.speed === 0
                    text: qsTr("CLEAR DTC")
                    onClicked: vcuController.clearDtc()
                }
            }
        }
    }
}
