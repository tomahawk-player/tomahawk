import QtQuick 1.1

Rectangle {
    id: root
    height: contentRow.height + defaultFontHeight / 2
    width: contentRow.width + defaultFontHeight / 2

    property alias text: buttonText.text
    property alias imageSource: image.source
    property bool enabled: true

    color: "transparent"
    border.width: defaultFontHeight / 16
    border.color: buttonMouseArea.containsMouse ? "lightblue" : "transparent"
    radius: defaultFontHeight / 4

    signal clicked()

    Row {
        id: contentRow
        spacing: defaultFontHeight / 4
        width: childrenRect.width
        height: childrenRect.height
        anchors.centerIn: parent
        Image {
            id: image
            height: defaultFontHeight
            width: source.length == 0 ? 0 : height
        }

        Text {
            id: buttonText
            color: root.enabled ? "black" : "grey"
        }
    }

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: root.enabled
        enabled: root.enabled
        onClicked: root.clicked();
    }
}
