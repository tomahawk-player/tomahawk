import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root
    anchors.fill: parent

    signal done(string text)

    function createStation(artist) {
        mainView.startStationFromArtist(artist)
        root.done(artist)
    }

    Column {
        id: upperColumn
        anchors.fill: parent
        anchors.margins: defaultFontHeight
        spacing: defaultFontHeight

        HeaderLabel {
            id: headerText
            text: "Pick one of your top artists,"
        }

        Item {
            height: parent.height - headerText.height*2 - artistInputField.height - parent.spacing * 3
            width: parent.width
            ArtistView {
                id: artistView
                height: parent.height
                width: parent.width
                model: artistChartsModel
                clip: true
                cellWidth: defaultFontHeight * 12
                cellHeight: defaultFontHeight * 12
                spacing: defaultFontHeight / 2

                onItemClicked: {
                    createStation(artistChartsModel.itemFromIndex(index).artistName);
                }
            }
            ScrollBar {
                listView: artistView
            }
        }

        HeaderLabel {
            text: "Or enter an artist name"
        }

        Row {
            height: artistInputField.height
            width: Math.min(defaultFontHeight * 30, parent.width)
            spacing: defaultFontHeight * 0.5
            anchors.horizontalCenter: parent.horizontalCenter

            InputField {
                id: artistInputField
                width: parent.width - createFromInputButton.width - parent.spacing

                onAccepted: createStation(text)
            }

            PushButton {
                id: createFromInputButton
                text: "Create station"
                enabled: artistInputField.text.length > 2
                onClicked: createStation(artistInputField.text)
            }
        }
    }
}
