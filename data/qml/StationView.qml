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
                anchors.right: parent.right
                anchors.top: parent.top
                height: parent.height
                width: parent.width
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

                states: [
                    State {
                        name: "empty"; when: mainView.loading
                        PropertyChanges {
                            target: coverView
                            anchors.rightMargin: -coverView.width
                            anchors.topMargin: - coverView.height
                            scale: 0
                        }
                    }
                ]
                transitions: [
                    Transition {
                        from: "empty"
                        to: "*"
                        NumberAnimation {
                            properties: "anchors.topMargin,anchors.rightMargin,scale"
                            duration: 1000
                            easing.type: Easing.OutQuad
                        }
                    }

                ]
//                Behavior on anchors.topMargin {
//                    NumberAnimation { duration: 500 }
//                }
//                Behavior on anchors.rightMargin {
//                    NumberAnimation { duration: 500 }
//                }
//                Behavior on scale {
//                    NumberAnimation { duration: 500 }
//                }

            }
            BusyIndicator {
                id: busyIndicator
                anchors.centerIn: parent
                height: defaultFontHeight * 4
                width: height

                opacity: mainView.loading ? 1 : 0
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
