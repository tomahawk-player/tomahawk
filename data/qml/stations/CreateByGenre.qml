import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root
    anchors.fill: parent

    signal done(string text)

    function createStation(genre) {
        mainView.startStationFromGenre(genre)
        root.done(genre)
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
            text: "Enter a genre,"
        }

        Row {
            width: Math.min(defaultFontHeight * 30, parent.width)
            height: parent.height * 0.2
            spacing: defaultFontHeight * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            z: 2

            InputField {
                id: genreInputField
                width: parent.width - createFromInputButton.width - parent.spacing
                completionModel: allGenres

                onAccepted: createStation(text);
            }

            PushButton {
                id: createFromInputButton
                text: "Create station"
                height: genreInputField.height
                enabled: genreInputField.text.length > 2
                onClicked: createStation(genreInputField.text)
            }
        }

        HeaderLabel {
            text: "Or, pick one of your most listened genres"
        }

        Item {
            height: parent.height - y
            width: parent.width
            clip: true
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
