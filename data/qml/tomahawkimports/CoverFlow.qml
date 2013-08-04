import QtQuick 1.1
import tomahawk 1.0

ListView {
    id: coverView

    property color backgroundColor: "black"
    property int coverSize: height / 2

    // emitted when a cover is clicked
    signal itemClicked(int index)

    // emitted when a cover is clicked
    signal itemPlayPauseClicked(int index)

    preferredHighlightBegin: (width / 2) - (coverSize / 4)
    preferredHighlightEnd: preferredHighlightBegin + coverSize / 2
    snapMode: ListView.SnapToItem
    highlightRangeMode: ListView.StrictlyEnforceRange
    highlightMoveDuration: 200

    property bool itemHovered: false
    orientation: ListView.Horizontal

    delegate: Item {
        id: delegateItem
        height: parent.height
        width: coverView.coverSize / 2
        anchors.verticalCenter: ListView.view.verticalCenter

        property real distanceFromLeftEdge: -coverView.contentX + index*width
        property real distanceFromRightEdge: coverView.contentX + coverView.width - (index+1)*width
        property real distanceFromEdge: Math.max(distanceFromLeftEdge, distanceFromRightEdge)

        scale: 2 - (distanceFromEdge / (coverView.width))

        property double itemBrightness: (1.3 - (distanceFromEdge / (coverView.width))) - ((coverView.itemHovered && !coverDelegate.containsMouse) ? .4 : 0)
        property double itemOpacity: coverView.itemHovered && !coverDelegate.containsMouse ? 0.4 : 1
        property int _origZ

        z: -Math.abs(currentIndex - index)

        CoverImage {
            id: coverDelegate
            height: coverView.coverSize / 2
            width: parent.width
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
            }

            showLabels: true
            showMirror: false
            artistName: model.artistName
            trackName: model.trackName
            artworkId: model.coverID
            showPlayButton: true
            currentlyPlaying: isPlaying
            smooth: true

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
    }
}
