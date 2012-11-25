import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    border.width: 4
    border.color: "white"
    radius: height / 2
    color: buttonMouseArea.containsMouse ? "#22ffffff" : "black"

    property string text

    signal clicked()

    Behavior on color {
        ColorAnimation { duration: 200 }
    }

    Text {
        anchors.centerIn: parent
        text: parent.text
        color: "white"
        font.pixelSize: parent.height * .75
        font.bold: true
    }
    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: parent.clicked()
    }
}
