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
        anchors.horizontalCenter: parent.horizontalCenter
        height: parent.height
        width: defaultFontHeight * 30
        anchors.bottomMargin: defaultFontHeight
        spacing: defaultFontHeight

        HeaderLabel {
            id: headerText
            text: "Enter or pick an artist"
        }

        Row {
            height: artistInputField.height
            width: parent.width
            spacing: defaultFontHeight * 0.5

            InputField {
                id: artistInputField
                width: parent.width - createFromInputButton.width - parent.spacing

                onAccepted: createStation(text)
            }

            PushButton {
                id: createFromInputButton
                text: "Go!"
                enabled: artistInputField.text.length > 2
                onClicked: createStation(artistInputField.text)
            }
        }

        Item {
            height: parent.height - headerText.height - artistInputField.height - parent.spacing * 3
            width: parent.width
            ArtistView {
                id: artistView
                height: parent.height
                width: parent.width
                model: artistChartsModel
                clip: true
                delegateHeight: defaultFontHeight * 6

                onItemClicked: {
                    createStation(artistChartsModel.itemFromIndex(index).artistName);
                }
            }
            ScrollBar {
                listView: artistView
            }
        }
    }
}
