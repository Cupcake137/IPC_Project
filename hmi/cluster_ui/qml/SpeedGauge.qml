import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property real speed: 0
    property real displaySpeed: speed
    property color accentColor: "#20c9d8"
    Behavior on displaySpeed { NumberAnimation { duration: 210; easing.type: Easing.OutCubic } }
    onDisplaySpeedChanged: canvas.requestPaint()
    onAccentColorChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        function point(cx, cy, radius, angle) {
            return Qt.point(cx + Math.cos(angle) * radius, cy + Math.sin(angle) * radius)
        }
        onPaint: {
            var ctx = getContext("2d")
            var cx = width / 2
            var cy = height / 2 + 12
            // Leave enough room for the 0-120 labels around the arc.
            var radius = Math.min(width, height) * 0.29
            var start = Math.PI * 0.75
            var span = Math.PI * 1.5
            var value = Math.max(0, Math.min(120, root.displaySpeed))
            ctx.reset(); ctx.lineCap = "round"
            ctx.beginPath(); ctx.arc(cx, cy, radius, start, start + span, false)
            ctx.lineWidth = 6; ctx.strokeStyle = "#555b60"; ctx.stroke()
            if (value > 0) {
                ctx.beginPath(); ctx.arc(cx, cy, radius, start, start + span * value / 120, false)
                ctx.lineWidth = 7; ctx.strokeStyle = root.accentColor; ctx.stroke()
            }
            for (var mark = 0; mark <= 120; mark += 5) {
                var angle = start + span * mark / 120
                var major = mark % 20 === 0
                var p1 = point(cx, cy, radius + (major ? 14 : 10), angle)
                var p2 = point(cx, cy, radius + (major ? 25 : 17), angle)
                ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y)
                ctx.lineWidth = major ? 2 : 1; ctx.strokeStyle = major ? "#e7eaec" : "#777e83"; ctx.stroke()
                if (major) {
                    var label = point(cx, cy, radius + 39, angle)
                    ctx.fillStyle = "#d6dadd"; ctx.font = "13px sans-serif"
                    ctx.textAlign = "center"; ctx.textBaseline = "middle"
                    ctx.fillText(mark.toString(), label.x, label.y)
                }
            }
        }
    }
    Column {
        anchors.centerIn: parent; anchors.verticalCenterOffset: 10; spacing: -2
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Math.round(root.displaySpeed); color: "#f7f8f9"
            font.pixelSize: 58; font.weight: Font.Light
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "km/h"; color: root.accentColor
            font.pixelSize: 20; font.bold: true
            Behavior on color { ColorAnimation { duration: 220 } }
        }
    }
}
