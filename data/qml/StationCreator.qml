import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: stationCreator

    property int spacing: width  * .05

    Flickable {
        id: flickArea
        width: parent.width - stationCreator.spacing
        anchors.fill: parent
        anchors.margins: stationCreator.spacing
        contentHeight: stationCreator.height
        contentWidth: width
        clip: true
        interactive: false

        Behavior on contentY {
            NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
        }

        function moveDown() {
            contentY = contentY + mainGrid.rowHeight + mainGrid.spacing
        }

        function moveUp() {
            contentY = contentY - mainGrid.rowHeight - mainGrid.spacing
        }

        Grid {
            id: mainGrid
            width: parent.width
            spacing: stationCreator.spacing
            columns: 2
            height: rowHeight * 3 + spacing * 2

            property int rowHeight: flickArea.height / 2

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

                    model: styleModel//generator.styles()

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











            Item {
                height: parent.rowHeight
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
                        rootView.startStationFromYear(decade)
                        stationListView.incrementCurrentIndex();
                    }

                    Text { text: "60s"; font.pointSize: 20; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "70s"; font.pointSize: 22; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "80s"; font.pointSize: 24; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "90s"; font.pointSize: 26; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                    Text { text: "00s"; font.pointSize: 28; color: "white"; MouseArea{ anchors.fill: parent; onClicked: yearsRow.decadeClicked(parent.text)}}
                }
            }
            Item {
                height: parent.rowHeight
                width: parent.width * 0.25
                Text {
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
                        rootView.startStationFromYear(text)
                        stationListView.incrementCurrentIndex();
                    }
                }

                RoundedButton {
                    text: ">"
                    height: orText.height * 3
                    width: height
                    anchors { horizontalCenter: yearInputField.horizontalCenter
                        top: yearInputField.bottom; topMargin: orText.height }
                    onClicked: yearInputField.accepted(yearInputField.text)
                }

            }

        }
    }


    RoundedButton {
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
        }
        height: orText.height * 3
        width: height * 3
        text: "^"
        onClicked: flickArea.moveUp()
        opacity: flickArea.contentY > 0 ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }
    RoundedButton {
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        height: orText.height * 3
        width: height * 3
        text: "v"
        onClicked: flickArea.moveDown()
        opacity: flickArea.contentY < mainGrid.rowHeight ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }
}
