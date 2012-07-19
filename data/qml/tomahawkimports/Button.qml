import QtQuick 1.1

Rectangle {
    id: root
    color:  buttonMouseArea.containsMouse ? "blue" : "gray"
    border.width: 2
    border.color: "white"
    radius: height/2
    height: buttonText.height * 1.2
    width: buttonText.width * 1.5

    property alias text: buttonText.text
    property color textColor: "white"

    signal clicked()

    Text {
        id: buttonText
        anchors.centerIn: parent
        color: root.textColor
    }

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked();
    }
}
