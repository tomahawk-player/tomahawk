import QtQuick 1.1
//import tomahawk 1.0
import "tomahawkimports"

Rectangle {
    anchors.fill: parent
    color: "black"

    Text {
        id: fontMetrics
        text: "Here's some sample text"
        opacity: 0
    }

    GridView {
        id: gridView
        anchors.fill: parent
        //anchors.rightMargin: scrollBar.width

        cellHeight: cellWidth
        cellWidth: calculateCoverSize(gridView.width - 3)

        cacheBuffer: cellHeight * 5

        function calculateCoverSize(rectWidth) {
            var itemWidth = fontMetrics.width;
            var itemsPerRow = Math.max( 1, Math.floor( rectWidth / itemWidth ) );

            var remSpace = rectWidth - ( itemsPerRow * itemWidth );
            var extraSpace = remSpace / itemsPerRow;
            return itemWidth + extraSpace;

        }

        model: mainModel

        delegate: CoverImage {
            height: gridView.cellHeight// * 0.95
            width: gridView.cellWidth// * 0.95

            showLabels: true
            showMirror: false
            artistName: model.artistName
            trackName: model.trackName
            artworkId: model.coverID
            showPlayButton: true
            currentlyPlaying: isPlaying
            smooth: true

            onClicked: {
                rootView.onItemClicked(index)
            }
            onPlayClicked: {
                rootView.onItemPlayClicked(index)
            }
        }
    }

    ScrollBar {
        id: scrollBar
        listView: gridView
        orientation: Qt.Vertical
        margin: -width
    }
}
