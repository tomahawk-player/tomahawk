import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    id: root
    border.width: 4
    border.color: enabled ? "white" : "grey"
    radius: height / 2
    color: (buttonMouseArea.containsMouse && enabled) ? "#22ffffff" : "black"
    opacity: hidden ? 0 : 1

    height: defaultFontHeight * 2
    width: height

    property string text
    property bool enabled: true
    property bool hidden: false

    signal clicked()

    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }

    Behavior on color {
        ColorAnimation { duration: 200 }
    }

    Text {
        anchors.centerIn: parent
        text: parent.text
        color: root.border.color
        font.pixelSize: parent.height * .75
        font.bold: true
    }
    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        onClicked: parent.clicked()
    }
}
