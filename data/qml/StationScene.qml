import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: "list"

    ListModel {
        id: modeModel
        ListElement { label: "By Artist"; image: "../images/artist-placeholder-grid.png"; creatorState: "artist" }
        ListElement { label: "By Genre"; image: "../images/album-placeholder-grid.png"; creatorState: "genre" }
        ListElement { label: "By Year"; image: "image://albumart/foobar"; creatorState: "year" }
    }

    ListModel {
        id: styleModel
        ListElement { modelData: "acoustic" }
        ListElement { modelData: "alternative" }
        ListElement { modelData: "alternative rock" }
        ListElement { modelData: "classic" }
        ListElement { modelData: "folk" }
        ListElement { modelData: "indie" }
        ListElement { modelData: "pop" }
        ListElement { modelData: "rock" }
        ListElement { modelData: "hip-hop" }
        ListElement { modelData: "punk" }
        ListElement { modelData: "grunge" }
        ListElement { modelData: "indie" }
        ListElement { modelData: "electronic" }
        ListElement { modelData: "country" }
        ListElement { modelData: "jazz" }
        ListElement { modelData: "psychodelic" }
        ListElement { modelData: "soundtrack" }
        ListElement { modelData: "reggae" }
        ListElement { modelData: "house" }
        ListElement { modelData: "drum and base" }
    }

    ListModel {
        id: dummyArtistModel
        ListElement { modelData: "Pink Floyd" }
        ListElement { modelData: "Tool" }
        ListElement { modelData: "Cake" }
        ListElement { modelData: "Metallica" }
        ListElement { modelData: "Red Hot Chili Peppers" }
        ListElement { modelData: "Korn" }
        ListElement { modelData: "Prodigy" }
        ListElement { modelData: "Otto Waalkes" }
    }

    VisualItemModel {
        id: stationVisualModel

        StationCreatorPage1 {
            height: scene.height
            width: scene.width
            model: modeModel

            onItemClicked: {
                stationCreator.state = modeModel.get(index).creatorState
                stationListView.incrementCurrentIndex()
            }
        }

        StationCreator {
            id: stationCreator
            height: scene.height
            width: scene.width

            onBack: stationListView.decrementCurrentIndex()

            onNext: stationListView.incrementCurrentIndex()
        }

        StationView {
            coverSize: Math.min(scene.height, scene.width) / 2
            height: scene.height
            width: scene.width
//            visible: stationListView.currentIndex == 1

            onBackClicked: stationListView.decrementCurrentIndex()
        }

    }

    ListView {
        id: stationListView
        anchors.fill: parent
        contentHeight: scene.height
        contentWidth: scene.width
        orientation: ListView.Horizontal
        model: stationVisualModel
        interactive: false
        highlightMoveDuration: 300

        onHeightChanged: {
            contentHeight = scene.height
        }
        onWidthChanged: {
            contentWidth = scene.width
        }
    }
}
