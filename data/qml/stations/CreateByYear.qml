import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root
    anchors.fill: parent

    signal done(string text)

    function createStationFromYear(year) {
        mainView.startStationFromYear(year)
        root.done(year)
    }

    function createStationFromTo(yearFrom, yearTo) {
        mainView.startStationFromTo(yearFrom, yearTo)
        root.done(yearFrom + " to " + yearTo)
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
            text: "Enter a year or pick a range"
        }

        Row {
            height: yearInputField.height
            width: parent.width
            spacing: defaultFontHeight * 0.5

            Text {
                text: "Year:"
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
            }

            InputField {
                id: yearInputField
                width: parent.width - createFromInputButton.width - parent.spacing

                onAccepted: createStation(text)
            }
        }

        DoubleSlider {
            id: yearSlider
            width: parent.width
            height: defaultFontHeight * 4
            min: 1960
            max: new Date().getFullYear()
            lowerSliderPos: 1990
            upperSliderPos: 2010
            minMaxLabelsVisible: false
            opacity: yearInputField.text.length > 0 ? 0.3 : 1

            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }

        PushButton {
            id: createFromInputButton
            text: "Go!"
            enabled: yearInputField.text.length == 0 || (yearInputField.text >= yearSlider.min && yearInputField.text <= yearSlider.max)
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: {
                if (yearInputField.text.length > 0) {
                    createStationFromYear(yearInputField.text)
                } else {
                    createStationFromTo(yearSlider.lowerSliderPos, yearSlider.upperSliderPos)
                }
            }

            // TODO: move some disabled look/animation to the button itself
            opacity: enabled ? 1 : 0.3
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }
}
