import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

PathView {
    id: coverView

    // The start coordinates for the covers
    // Default is left, centered in height
    property int pathStartX: 0
    property int pathStartY: height

    // The size of the covers in the path
    property int coverSize: 100

    property color backgroundColor: "black"

    // emitted when a cover is clicked
    signal itemClicked(int index)

    // emitted when a cover is clicked
    signal itemPlayPauseClicked(int index)

    preferredHighlightBegin: 0.2 // scene.width / 11000
    preferredHighlightEnd: preferredHighlightBegin
    pathItemCount: 5
    //highlightMoveDuration: 500

    delegate: CoverImage {
        id: coverDelegate
        height: root.coverSize
        width: root.coverSize
        backgroundColor: coverView.backgroundColor

        showLabels: true
        showMirror: true
        artistName: model.artistName
        trackName: model.trackName
        artworkId: model.coverID
        showPlayButton: true
        currentlyPlaying: isPlaying

        scale: PathView.itemScale
        itemBrightness: PathView.itemBrightness
        opacity: PathView.itemOpacity
        z: coverView.width - x

        property int _origX
        property int _origY
        property int _origZ

        onPlayClicked: {
            console.log("***************")
            coverView.itemPlayPauseClicked(index)
        }

        onClicked: {
            coverView.itemClicked(index)
        }

        function newZ() {
            return z;
        }

        onContainsMouseChanged: {
            if (containsMouse) {
                _origX = x;
                _origY = y;
                _origZ = z;
            }
        }

        states: [
            State {
                name: "hovered"; when: coverDelegate.containsMouse && index !== currentIndex
                PropertyChanges {
                    target: coverDelegate
                    x: _origX + coverDelegate.width * coverDelegate.scale / 2
                    z: _origZ
                }
            }
        ]
        transitions: [
            Transition {
                NumberAnimation {
                    properties: "x"
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
        PathAttribute { name: "itemScale"; value: 2 }

        PathLine { x: coverView.width / 4; y: coverView.height / 4 * 3}
        PathPercent { value: 0.1 }
        PathAttribute { name: "itemOpacity"; value: 0 }
        PathAttribute { name: "itemBrightness"; value: 1 }
        PathAttribute { name: "itemScale"; value: 1.5 }

        PathLine { x: coverView.width / 2; y: coverView.height / 2}
        PathPercent { value: 0.2 }
        PathAttribute { name: "itemOpacity"; value: 1 }
        PathAttribute { name: "itemBrightness"; value: 1 }
        PathAttribute { name: "itemScale"; value: 1 }

        PathLine { x: coverView.width; y: 0 }
        PathPercent { value: 1 }
        PathAttribute { name: "itemOpacity"; value: 1 }
        PathAttribute { name: "itemBrightness"; value: 0 }
        PathAttribute { name: "itemScale"; value: 0.1 }
    }

}
