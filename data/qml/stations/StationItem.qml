import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: stationItem

    CoverFlow {
        id: coverView
        interactive: false
        anchors.fill: parent

        backgroundColor: scene.color

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        onItemPlayPauseClicked: {
            mainView.playItem(index)
        }

        onItemClicked: {
            mainView.playItem(index)
        }
    }
    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        height: defaultFontHeight * 4
        width: height
//        count: 12

        opacity: mainView.loading ? 1 : 0
        running: mainView.loading
    }

}
