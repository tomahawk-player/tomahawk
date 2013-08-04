import QtQuick 1.1
import tomahawk 1.0

FocusScope {
    id: root

    property alias text: messageText.text
    property alias inputText: inputField.text

    property real arrowPosition: 1

    height: contentColumn.height + defaultFontHeight * 2

    signal accepted()
    signal rejected()

    onFocusChanged: {
        if (focus) {
            inputField.forceActiveFocus()
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: -999999999
        hoverEnabled: root.opacity > 0
        enabled: root.opacity > 0
    }

    Item {
        id: backgroundItem
        anchors.fill: parent
        opacity: 0.9

        Rectangle {
            id: background
            anchors.fill: parent
            color: "white"
            border.color: "black"
            border.width: defaultFontHeight / 10
            radius: defaultFontHeight / 2
            anchors.topMargin: defaultFontHeight / 4
        }

        Item {
            clip: true
            anchors.bottom: backgroundItem.top
            anchors.bottomMargin: -background.border.width*3
            height: defaultFontHeight
            width: height
            x: defaultFontHeight - arrowRect.width/2 + (root.width - defaultFontHeight*2) * arrowPosition
            Rectangle {
                id: arrowRect
                height: defaultFontHeight / 1.8
                width: height
                rotation: 45
                color: "white"
                anchors.centerIn: parent
                anchors.verticalCenterOffset: parent.height/2
                border.color: "black"
                border.width: defaultFontHeight / 10
            }
        }
    }

    Column {
        id: contentColumn
        width: parent.width - defaultFontHeight
        height: childrenRect.height
        anchors.centerIn: parent
        anchors.verticalCenterOffset: defaultFontHeight / 4
        spacing: defaultFontHeight / 2

        Row {
            width: parent.width
            height: childrenRect.height
            spacing: defaultFontHeight / 2
            Text {
                id: messageText
                wrapMode: Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
            }
            InputField {
                id: inputField
                width: parent.width - x
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Row {
            height: childrenRect.height
            anchors.right: parent.right
            spacing: defaultFontHeight
            Button {
                text: "OK"
                imageSource: "qrc:///data/images/ok.svg"
                enabled: inputField.text.length > 0
                onClicked: root.accepted()
            }
            Button {
                text: "Cancel"
                imageSource: "qrc:///data/images/cancel.svg"
                onClicked: {
                    inputField.text = ""
                    root.rejected()
                }
            }
        }
    }
}
