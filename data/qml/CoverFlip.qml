import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

PathView {
    id: coverView

    // The start coordinates for the covers
    // Default is left, centered in height
    property int pathStartX: coverSize
    property int pathStartY: height / 2

    // The size of the covers in the path
    property int coverSize: 100

    // emitted when a cover is clicked
    signal itemClicked(int index)

    // emitted when a cover is clicked
    signal itemPlayPauseClicked(int index)

    preferredHighlightBegin: 0.2 // scene.width / 11000
    preferredHighlightEnd: preferredHighlightBegin
    pathItemCount: 5
    //highlightMoveDuration: 500

    delegate: CoverImage {
        height: root.coverSize
        width: root.coverSize

        showLabels: true
        showMirror: true
        artistName: model.artistName
        trackName: model.trackName
        artworkId: model.coverID
        showPlayButton: true
        currentyPlayed: mode.itemFromIndex(index).isPlaying

        scale: PathView.itemScale
        itemBrightness: PathView.itemBrightness
        opacity: PathView.itemOpacity
        z: -x

        onPlayClicked: {
            coverView.itemPlayPauseClicked(index)
        }

        onClicked: {
            coverView.itemClicked(index)
        }
    }

    path: Path {
        startX: coverView.pathStartX
        startY: coverView.pathStartY

        PathAttribute { name: "itemOpacity"; value: 1 }
        PathAttribute { name: "itemBrightness"; value: 1 }
        PathAttribute { name: "itemScale"; value: 1 }
//            PathLine { x: coverView.pathStartX * 0.9 ; y: coverView.pathStartY * 0.9 }
//            PathPercent { value: .2 }
//            PathAttribute { name: "itemOpacity"; value: 1 }
//            PathAttribute { name: "itemBrightness"; value: 1 }
//            PathAttribute { name: "itemScale"; value: 1 }
//            PathLine { x: coverView.pathStartX * .5; y: coverView.pathStartY * .5}
//            PathPercent { value: .3 }
//            PathAttribute { name: "itemOpacity"; value: 1 }
//            PathAttribute { name: "itemBrightness"; value: 1 }
//            PathAttribute { name: "itemScale"; value: 0.5 }
//            //            PathLine { x: coverView.pathStartX * .25 ; y: coverView.pathStartY * .25 }
//            //            PathPercent { value: .75 }
//            //            PathAttribute { name: "itemOpacity"; value: 1 }
//            //            PathAttribute { name: "itemBrightness"; value: .5 }
//            //            PathAttribute { name: "itemScale"; value: 0.4 }
        PathLine { x: root.width; y: 0 }
        PathPercent { value: 1 }
        PathAttribute { name: "itemOpacity"; value: 1 }
        PathAttribute { name: "itemBrightness"; value: 0 }
        PathAttribute { name: "itemScale"; value: 0.1 }
    }

}
