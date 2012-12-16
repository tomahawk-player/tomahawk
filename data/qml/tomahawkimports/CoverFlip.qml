import QtQuick 1.1
import tomahawk 1.0

PathView {
    id: coverView

    // The start coordinates for the covers
    // Default is left, centered in height
    property int pathStartX: 0
    property int pathStartY: height

    // The size of the covers in the path
    property int coverSize: height

    property color backgroundColor: "black"

    // emitted when a cover is clicked
    signal itemClicked(int index)

    // emitted when a cover is clicked
    signal itemPlayPauseClicked(int index)

    preferredHighlightBegin: 0.2 // scene.width / 11000
    preferredHighlightEnd: preferredHighlightBegin
    pathItemCount: 5
    //highlightMoveDuration: 500

    property bool itemHovered: false

    delegate: Item {
        id: delegateItem
        height: coverView.coverSize
        width: coverView.coverSize

        scale: PathView.itemScale
        //        itemBrightness: PathView.itemBrightness - ((coverView.itemHovered && !coverDelegate.containsMouse) ? .4 : 0)
        property double itemBrightness: PathView.itemBrightness
        property double itemOpacity: PathView.itemOpacity
        property int _origZ

        z: coverView.width - x

        CoverImage {
            id: coverDelegate
            height: coverView.coverSize
            width: coverView.coverSize
            anchors {
                top: parent.top
                right: parent.right
            }

            backgroundColor: coverView.backgroundColor

            showLabels: true
            showMirror: true
            artistName: model.artistName
            trackName: model.trackName
            artworkId: model.coverID
            showPlayButton: true
            currentlyPlaying: isPlaying
            smooth: true

            //        itemBrightness: PathView.itemBrightness - ((coverView.itemHovered && !coverDelegate.containsMouse) ? .4 : 0)
            itemBrightness: coverDelegate.containsMouse ? 1 : parent.itemBrightness * (coverView.itemHovered ? .5 : 1)
            opacity: parent.itemOpacity
            z: coverView.width - x

            onPlayClicked: {
                console.log("***************")
                coverView.itemPlayPauseClicked(index)
            }

            onClicked: {
                coverView.itemClicked(index)
            }

            onContainsMouseChanged: {
                if (containsMouse) {
                    delegateItem._origZ = delegateItem.z;
                    coverView.itemHovered = true
                } else {
                    coverView.itemHovered = false
                }
            }


        }
        states: [
            State {
                name: "hovered"; when: coverDelegate.containsMouse && !coverView.moving && index !== currentIndex
                PropertyChanges {
                    target: delegateItem
                    width: coverView.coverSize * 2
                    z: delegateItem._origZ
                }
            }
        ]
        transitions: [
            Transition {
                NumberAnimation {
                    properties: "width"
                    duration: 300
                    easing.type: Easing.InOutSine
                }

            }
        ]
    }

    path: Path {
        startX: coverView.pathStartX
        startY: coverView.pathStartY

        PathAttribute { name: "itemOpacity"; value: 0 }
        PathAttribute { name: "itemBrightness"; value: 0 }
        PathAttribute { name: "itemScale"; value: 1.3 }

        PathLine { x: coverView.width / 4; y: coverView.height / 4 * 3}
        PathPercent { value: 0.1 }
        PathAttribute { name: "itemOpacity"; value: 0 }
        PathAttribute { name: "itemBrightness"; value: 1 }
        PathAttribute { name: "itemScale"; value: 1.0 }

        PathLine { x: coverView.width / 2; y: coverView.height / 2}
        PathPercent { value: 0.2 }
        PathAttribute { name: "itemOpacity"; value: 1 }
        PathAttribute { name: "itemBrightness"; value: 1 }
        PathAttribute { name: "itemScale"; value: 0.5 }

        PathLine { x: coverView.width; y: 0 }
        PathPercent { value: 1 }
        PathAttribute { name: "itemOpacity"; value: 1 }
        PathAttribute { name: "itemBrightness"; value: 0 }
        PathAttribute { name: "itemScale"; value: 0.1 }
    }

}
