import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root
    anchors.fill: parent

    signal done()

    function createStation(artist) {
        mainView.startStationFromArtist(artist)
        root.done()
    }

    Column {
        id: upperColumn
        anchors.centerIn: parent
        height: parent.height
        width: defaultFontHeight * 30
        anchors.bottomMargin: defaultFontHeight
        spacing: defaultFontHeight

        HeaderLabel {
            id: headerText
            text: "Create station by artist..."
        }

        Row {
            height: artistInputField.height
            width: parent.width
            spacing: defaultFontHeight

            InputField {
                id: artistInputField
                width: parent.width - createFromInputButton.width - parent.spacing

                onAccepted: createStation(text)
            }

            RoundedButton {
                id: createFromInputButton
                text: ">"
                height: artistInputField.height
                enabled: artistInputField.text.length > 2
                onClicked: createStation(artistInputField.text)
            }
        }

        ArtistView {
            height: parent.height - headerText.height - artistInputField.height - parent.spacing * 3
            width: parent.width
            model: artistChartsModel
            clip: true
            delegateHeight: defaultFontHeight * 6

            onItemClicked: {
                createStation(artistChartsModel.itemFromIndex(index).artistName);
            }
        }
    }
}
