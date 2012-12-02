import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root
    anchors.fill: parent

    signal done()

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

    Column {
        id: upperColumn
        anchors.fill: parent
        anchors.bottomMargin: defaultFontHeight
        spacing: defaultFontHeight

        HeaderLabel {
            id: headerText
            text: "Create station by artist..."
        }

        Row {
            width: defaultFontHeight * 30
            height: artistInputField.height
            spacing: defaultFontHeight

            InputField {
                id: artistInputField
                width: parent.width - createFromInputButton.width - parent.spacing
            }

            RoundedButton {
                id: createFromInputButton
                text: ">"
                height: artistInputField.height
            }
        }

        ArtistView {
            height: parent.height - headerText.height - artistInputField.height
            width: defaultFontHeight * 30
            model: artistChartsModel
            clip: true
            delegateHeight: defaultFontHeight * 6

            onItemClicked: {
                mainView.startStationFromArtist(artistChartsModel.itemFromIndex(index).artistName)
                root.done()
            }
        }
    }
}
