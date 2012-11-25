import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: "list"

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


        Item {
            height: scene.height
            width: scene.width
            Grid {
                anchors.fill: parent
                anchors.margins: spacing
                spacing: width * .1
                columns: 3

                property int rowHeight: height / 2

                Item {
                    height: parent.rowHeight
                    width: parent.width / 2
                    GridView {
                        id: gridView
                        anchors.fill: parent
                        anchors.margins: cellWidth / 2
                        model: dummyArtistModel

                        cellWidth: gridView.width / 4 - 1 // -1 to make sure there is space for 4 items even with rounding error
                        cellHeight: cellWidth

                        delegate: Item {
                            height: gridView.cellHeight * .9
                            width: height

                            CoverImage {
                                artistName: modelData
                                anchors.fill: parent

                                onClicked: {
                                    rootView.startStationFromArtist(modelData);
                                    stationListView.incrementCurrentIndex();
                                }
                            }
                        }
                    }
                }
                Item {
                    height: parent.rowHeight
                    width: orText.width
                    Text {
                        id: orText
                        anchors.centerIn: parent
                        text: "or"
                        color: "white"
                    }
                }
                Item {
                    height: parent.rowHeight
                    width: parent.width / 4
                    InputField {
                        anchors.centerIn: parent
                        width: parent.width
                        onAccepted: {
                            rootView.startStationFromArtist(text)
                            stationListView.incrementCurrentIndex();
                        }
                    }
                }

                Item {
                    height: parent.rowHeight
                    width: parent.width / 2

                    TagCloud {
                        anchors.fill: parent
                        model: styleModel//generator.styles()
                        opacity: echonestStation.configured ? 0 : 1

                        onTagClicked: {
                            rootView.startStationFromGenre(item)
                            stationListView.incrementCurrentIndex();
                        }

                        Behavior on opacity {
                            NumberAnimation { duration: 300 }
                        }
                    }
                }
                Item {
                    height: parent.rowHeight
                    width: orText.width
                    Text {
                        text: "or"
                        color: "white"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Item {
                    height: parent.rowHeight
                    width: parent.width / 4
                    InputField {
                        anchors.centerIn: parent
                        width: parent.width
                        onAccepted: {
                            rootView.startStationFromGenre(text)
                            stationListView.incrementCurrentIndex();
                        }
                    }
                }
            }
        }

        StationView {
            coverSize: Math.min(scene.height, scene.width) / 2
            height: scene.height
            width: scene.width
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
        highlightMoveDuration: 200

    }
}
