import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: stationCreator
    state: "artist"

    property int spacing: width / 10

    signal back()
    signal next()

    Loader {
        id: loader
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: previousButton.top
            margins: parent.spacing
        }
    }

    RoundedButton {
        id: previousButton
        text: "<"
        height: spacing
        width: height
        anchors {
            left: parent.left
            bottom: parent.bottom;
            margins: stationCreator.spacing
        }
        onClicked: stationCreator.back()
    }

    RoundedButton {
        id: nextButton
        text: ">"
        height: spacing
        width: height
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: stationCreator.spacing
        }
        onClicked: {
            loader.item.createStation()
            stationCreator.next()
        }
    }

    states: [
        State {
            name: "artist"
            PropertyChanges { target: loader; sourceComponent: createByArtist }
        },
        State {
            name: "genre"
            PropertyChanges { target: loader; sourceComponent: createByGenre }
        },
        State {
            name: "year"
            PropertyChanges { target: loader; sourceComponent: createByYear }
        }

    ]

    Component {
        id: createByArtist

        Row {
            function createStation() {
                mainView.startStationFromArtist(artistInputField.text)
            }

            anchors.fill: parent
            spacing: stationCreator.spacing
            Item {
                height: parent.height
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
                                mainView.startStationFromArtist(modelData);
                                stationCreator.next()
                            }
                        }
                    }
                }

            }
            Item {
                height: parent.height
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
                        mainView.startStationFromArtist(text)
                        stationCreator.next()
                    }
                }

            }
        }
    }


    Component {
        id: createByGenre
        Row {
            function createStation() {
                mainView.startStationFromGenre(genreInputField.text)
            }

            anchors.fill: parent
            spacing: stationCreator.spacing
            Item {
                height: parent.height
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

                    model: styleModel//generator.styles()

                    onTagClicked: {
                        mainView.startStationFromGenre(item)
                        stationCreator.next()
                    }

                    Behavior on opacity {
                        NumberAnimation { duration: 300 }
                    }
                }
            }
            Item {
                height: parent.height
                width: parent.width * 0.25
                Text {
                    id: orText
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
                        mainView.startStationFromGenre(text)
                        stationCreator.next()
                    }
                }

            }
        }
    }
    Component {
        id: createByYear

        Row {
            anchors.fill: parent

            Item {
                height: parent.height
                width: parent.width * 0.7
                Text {
                    id: selectYearText
                    anchors { left: parent.left; right: parent.right; top: parent.top}
                    text: "Select a decade..."
                    color: "white"
                    font.bold: true
                }

                Row {
                    id: yearsRow
                    width: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: selectYearText.height

                    function decadeClicked(decade) {
                        mainView.startStationFromYear(decade)
                        stationCreator.next()
                    }

                    Text { text: "60s"; font.pointSize: 20; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "70s"; font.pointSize: 22; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "80s"; font.pointSize: 24; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "90s"; font.pointSize: 26; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "00s"; font.pointSize: 28; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                }
            }
            Item {
                height: parent.height
                width: parent.width * 0.25
                Text {
                    id: orText
                    text: "...or specify a year:"
                    color: "white"
                    anchors { left: parent.left; right: parent.right;
                        bottom: yearInputField.top; bottomMargin: height }
                    font.bold: true
                }
                InputField {
                    id: yearInputField
                    anchors.centerIn: parent
                    width: parent.width
                    onAccepted: {
                        mainView.startStationFromYear(text)
                        stationCreator.next()
                    }
                }
            }
        }
    }
}
