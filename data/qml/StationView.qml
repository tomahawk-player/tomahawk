import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: root
    property int coverSize

    signal configure()

    PathView {
        id: coverView
        anchors.fill: parent
        anchors.rightMargin: parent.width / 3

        preferredHighlightBegin: 0.2 // scene.width / 11000
        preferredHighlightEnd: preferredHighlightBegin
        pathItemCount: 5
        //highlightMoveDuration: 500

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        property int pathStartX: width / 2
        property int pathStartY: height / 2

        delegate: CoverImage {
            height: root.coverSize
            width: root.coverSize

            showLabels: false
            //artistName: model.artistName
            //trackName: model.trackName
            artworkId: model.coverID

            scale: PathView.itemScale
            itemBrightness: PathView.itemBrightness
            opacity: PathView.itemOpacity
            z: x

            onClicked: {
                if ( currentlyPlayedIndex !==-1 ) {
                    echonestStation.playItem( index )
                }
            }
        }

        path: Path {
            startX: coverView.pathStartX
            startY: coverView.pathStartY

            PathAttribute { name: "itemOpacity"; value: 0 }
            PathAttribute { name: "itemBrightness"; value: 0 }
            PathAttribute { name: "itemScale"; value: 1.5 }
            PathLine { x: coverView.pathStartX * 0.9 ; y: coverView.pathStartY * 0.9 }
            PathPercent { value: .2 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 1 }
            PathAttribute { name: "itemScale"; value: 1 }
            PathLine { x: coverView.pathStartX * .5; y: coverView.pathStartY * .5}
            PathPercent { value: .3 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 1 }
            PathAttribute { name: "itemScale"; value: 0.5 }
            //            PathLine { x: coverView.pathStartX * .25 ; y: coverView.pathStartY * .25 }
            //            PathPercent { value: .75 }
            //            PathAttribute { name: "itemOpacity"; value: 1 }
            //            PathAttribute { name: "itemBrightness"; value: .5 }
            //            PathAttribute { name: "itemScale"; value: 0.4 }
            PathLine { x: 0; y: 0 }
            PathPercent { value: 1 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 0 }
            PathAttribute { name: "itemScale"; value: 0.1 }
        }

    }

    Item {
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        anchors.margins: 50
        width: scene.width / 3

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

