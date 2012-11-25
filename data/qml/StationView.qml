import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: root
    property int coverSize

    signal backClicked()

    CoverFlip {
        id: coverView
        anchors.fill: parent

        backgroundColor: scene.color

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        onItemPlayPauseClicked: {
            rootView.playItem(index)
        }

        onItemClicked: {
            rootView.playItem(index)
        }

    }

    Item {
        anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
        anchors.margins: titleText.height * 3
        width: scene.width / 2

        Column {
            anchors { left: parent.left; top: parent.top; right: parent.right }
            Text {
                id: titleText
                color: "white"
                font.pointSize: 18
                width: parent.width
                elide: Text.ElideRight
                text: rootView.title
            }
            Text {
                color: "white"
                font.pointSize: 14
                font.bold: true
                width: parent.width
                elide: Text.ElideRight
                opacity: .8
                text: generator.summary
            }
        }
        Column {
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: scene.width / 3
            spacing: titleText.height * 2


            Rectangle {
                border.width: 4
                border.color: "white"
                height: titleText.height * 3
                width: height
                radius: height / 2
                color: backbuttonMouseArea.containsMouse ? "#22ffffff" : "black"
                Behavior on color {
                    ColorAnimation { duration: 200 }
                }

                Text {
                    anchors.centerIn: parent
                    text: "<"
                    color: "white"
                    font.pixelSize: parent.height * .75
                    font.bold: true
                }
                MouseArea {
                    id: backbuttonMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.backClicked()
                }
            }
            Rectangle {
                border.width: 4
                border.color: "white"
                height: titleText.height * 3
                width: height
                radius: height / 2
                color: addbuttonMouseArea.containsMouse ? "#22ffffff" : "black"
                Behavior on color {
                    ColorAnimation { duration: 200 }
                }
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    color: "white"
                    font.pixelSize: parent.height * .75
                    font.bold: true
                }
                MouseArea {
                    id: addbuttonMouseArea
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: root.backClicked()
                }
            }
        }
    }

}

