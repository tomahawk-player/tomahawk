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
                spacing: width * .05
                columns: 2

                property int rowHeight: height / 2

                Item {
                    height: parent.rowHeight
                    width: parent.width * 0.7
                    Text {
                        id: artistGridLabel
                        text: "Select an artist..."
                        anchors { left: parent.left; top: parent.top; right: parent.right }
                        color: "white"
                        font.bold: true
                    }

                    GridView {
                        id: gridView
                        anchors { left: parent.left; top: artistGridLabel.bottom; topMargin: artistGridLabel.height; right: parent.right; bottom: parent.bottom }
                        model: dummyArtistModel

                        cellWidth: Math.min(gridView.width / 4 - 1, gridView.height / 2) // -1 to make sure there is space for 4 items even with rounding error
                        cellHeight: cellWidth

                        delegate: Item {
                            height: gridView.cellHeight// * .9
                            width: height

                            CoverImage {
                                artistName: modelData
                                anchors.fill: parent
                                showPlayButton: true

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
                    width: parent.width * 0.25
                    Text {
                        id: orText
                        anchors { left: parent.left; right: parent.right;
                            bottom: artistInputField.top; bottomMargin: height }
                        text: "...or enter a name:"
                        color: "white"
                        font.bold: true
                    }

                    InputField {
                        id: artistInputField
                        anchors.centerIn: parent
                        width: parent.width
                        onAccepted: {
                            rootView.startStationFromArtist(text)
                            stationListView.incrementCurrentIndex();
                        }
                    }

                    RoundedButton {
                        text: ">"
                        height: orText.height * 3
                        width: height
                        anchors { horizontalCenter: artistInputField.horizontalCenter
                            top: artistInputField.bottom; topMargin: orText.height }
                        onClicked: artistInputField.accepted(artistInputField.text)
                    }

                }

                Item {
                    height: parent.rowHeight
                    width: parent.width * 0.7
                    Text {
                        id: selectGenreText
                        anchors { left: parent.left; right: parent.right; top: parent.top}
                        text: "Select a genre..."
                        color: "white"
                        font.bold: true
                    }

                    TagCloud {
                        anchors.fill: parent
                        anchors.topMargin: selectGenreText.height * 2
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
                    width: parent.width * 0.25
                    Text {
                        text: "...or enter your style:"
                        color: "white"
                        anchors { left: parent.left; right: parent.right;
                            bottom: genreInputField.top; bottomMargin: height }
                        font.bold: true
                    }
                    InputField {
                        id: genreInputField
                        anchors.centerIn: parent
                        width: parent.width
                        onAccepted: {
                            rootView.startStationFromGenre(text)
                            stationListView.incrementCurrentIndex();
                        }
                    }

                    RoundedButton {
                        text: ">"
                        height: orText.height * 3
                        width: height
                        anchors { horizontalCenter: genreInputField.horizontalCenter
                            top: genreInputField.bottom; topMargin: orText.height }
                        onClicked: genreInputField.accepted(genreInputField.text)
                    }

                }
            }
        }

        StationView {
            coverSize: Math.min(scene.height, scene.width) / 2
            height: scene.height
            width: scene.width
            visible: stationListView.currentIndex == 1

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
