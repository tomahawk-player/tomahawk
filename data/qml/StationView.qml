import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: root
    property int coverSize

    signal configure()


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
                text: rootView.title
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

    Button {
        id: configureButton
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        text: "configure"
        onClicked: root.configure();
    }

}

