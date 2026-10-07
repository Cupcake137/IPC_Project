import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    property url iconSource: ""
    property string label: ""
    property string value: ""
    width: 220; height: 22; spacing: 7
    Image { width: 17; height: 17; source: parent.iconSource; visible: source !== ""; fillMode: Image.PreserveAspectFit }
    Label { width: 76; text: parent.label; color: "#848c91"; font.pixelSize: 11; font.bold: true }
    Label { width: 113; text: parent.value; color: "#eef1f2"; font.pixelSize: 12; horizontalAlignment: Text.AlignRight }
}
