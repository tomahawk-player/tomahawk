import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"
import "stations"
Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: "list"

    FlexibleHeader {
        id: header
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }
        height: defaultFontHeight * 4
        width: parent.width
        icon: "../images/station.svg"
        title: mainView.title
        subtitle: generator.summary
        showSearchField: false
        showBackButton: stationListView.currentIndex > 0
        showNextButton: stationListView.currentIndex == 2
        nextButtonText: "Save"

        z: 1 //cover albumcovers that may leave their area

        onBackPressed: stationListView.decrementCurrentIndex()
        onNextPressed: stationListView.incrementCurrentIndex()
    }

    ListModel {
        id: modeModel
        ListElement { label: "By Artist"; image: "../../images/station-artist.svg"; creatorContent: "stations/CreateByArtist.qml" }
        ListElement { label: "By Genre"; image: "../../images/station-genre.svg"; creatorContent: "stations/CreateByGenre.qml" }
        ListElement { label: "By Year"; image: "../../images/station-year.svg"; creatorContent: "year" }
    }

    VisualItemModel {
        id: stationVisualModel

        StationCreatorPage1 {
            height: scene.height - header.height
            width: scene.width
            model: modeModel

            onItemClicked: {
                stationCreator.content = modeModel.get(index).creatorContent
                stationListView.incrementCurrentIndex()
            }
        }

        StationCreatorPage2 {
            id: stationCreator
            height: stationListView.height
            width: stationListView.width

            onNext: stationListView.incrementCurrentIndex()
        }

        Item {
            id: stationItem
            height: stationListView.height
            width: stationListView.width

            CoverFlip {
                id: coverView
                anchors.fill: parent
                interactive: false

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

                visible: mainView.loading
                running: mainView.loading
            }

        }

    }


    ListView {
        id: stationListView
        anchors {
            left: parent.left
            top: header.bottom
            right: parent.right
            bottom: parent.bottom
        }

        contentHeight: height
        contentWidth: width
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
