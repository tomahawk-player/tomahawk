import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root
    anchors.fill: parent

    signal done()

    function createStation(genre) {
        mainView.startStationFromGenre(genre)
        root.done()
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

    Column {
        id: upperColumn
        anchors.fill: parent
        anchors.bottomMargin: defaultFontHeight
        spacing: defaultFontHeight

        HeaderLabel {
            id: headerText
            text: "Create station by genre..."
        }

        Row {
            width: defaultFontHeight * 30
            height: artistInputField.height
            spacing: defaultFontHeight

            InputField {
                id: genreInputField
                width: parent.width - createFromInputButton.width - parent.spacing

                onAccepted: createStation(text);
            }

            RoundedButton {
                id: createFromInputButton
                text: ">"
                height: genreInputField.height
                enabled: genreInputField.text.length > 2
                onClicked: createStation(genreInputField.text)
            }
        }

        Item {
            height: parent.height - headerText.height - genreInputField.height
            width: parent.width
            TagCloud {
                anchors.fill: parent
                anchors.margins: parent.width / 6
                model: styleModel

                onTagClicked: {
                    root.createStation(tag);
                }
            }
        }
    }
}
