import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: root
    property int coverSize

    signal configure()


    CoverFlip {
        id: coverView
        anchors.fill: parent
        anchors.leftMargin: parent.width / 2

        model: dynamicModel

        onItemPlayPauseClicked: {
            rootView.playItem(index)
        }

    }

    Item {
        anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
        anchors.margins: 50
        width: scene.width / 2

        Column {
            anchors { left: parent.left; top: parent.top; right: parent.right }
            Text {
                color: "white"
                font.pointSize: 12
                width: parent.width
                elide: Text.ElideRight
                text: "Station:"
            }
            Text {
                color: "white"
                font.pointSize: 14
                font.bold: true
                width: parent.width
                elide: Text.ElideRight
                text: echonestStation.name
            }
        }

        Column {
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: scene.width / 3


            Text {
                color: "white"
                font.pointSize: 12
                width: parent.width
                elide: Text.ElideRight
                text: "Now Playing:"
                visible: currentlyPlayedIndex !== -1
            }
            Rectangle {
                height: image.height + image.height / 5
                width: image.width + startPlayingText.width * 1.2
                radius: height / 2
                border.width: 2
                border.color: "white"
                color: startPlayingMouseArea.containsMouse ? "blue" : "gray"
                visible: currentlyPlayedIndex === -1
                Image {
                    id: image
                    source: "../images/play-rest.png"
                    anchors.left: parent.left
                    anchors.margins: 10
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    id: startPlayingText
                    color: "white"
                    font.pointSize: 20
                    anchors.left: image.right
                    anchors.margins: height / 5
                    anchors.verticalCenter: parent.verticalCenter
                    //width: parent.width - 30 - image.width
                    elide: Text.ElideRight
                    text: "Start playing"
                }
                MouseArea {
                    id: startPlayingMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: echonestStation.playItem( 0 );
                }
            }

            Text {
                color: "white"
                font.pointSize: 16
                width: parent.width
                elide: Text.ElideRight
                text: currentlyPlayedIndex > -1 ? coverView.model.itemFromIndex( currentlyPlayedIndex ).name : ""
            }
            Text {
                color: "white"
                font.pointSize: 14
                width: parent.width
                elide: Text.ElideRight
                text: currentlyPlayedIndex > -1 ? coverView.model.itemFromIndex( currentlyPlayedIndex ).artistName : ""
            }
            Text {
                color: "white"
                font.pointSize: 14
                width: parent.width
                elide: Text.ElideRight
                text: currentlyPlayedIndex > -1 ? coverView.model.itemFromIndex( currentlyPlayedIndex ).albumName : ""
            }
        }
    }

    Button {
        id: configureButton
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        text: "configure"
        onClicked: root.configure();
    }

}

