import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: fineTuneView

    property color textColor: "white"

    signal done();

    Grid {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 50
        anchors.horizontalCenter: parent.horizontalCenter
        width: scene.width / 2
        spacing: 50
        columns: 2

        Text {
            color: fineTuneView.textColor
            text: "Name:"

        }
        InputField {
            text: echonestStation.name

            onAccepted: {
                print("text changed!!!")
                echonestStation.name = text;
            }
        }

        Text {
            id: tempoText
            text: "Tempo:"
            color: "white"
        }
        DoubleSlider {
            width: 500
            height: tempoText.height
            min: 0
            max: 500
            lowerSliderPos: echonestStation.minTempo
            upperSliderPos: echonestStation.maxTempo
            onValueChanged: echonestStation.setTempo( lowerSliderPos, upperSliderPos )
        }

        Text {
            id: hotnessText
            text: "Hotness:"
            color: "white"
        }
        DoubleSlider {
            width: 500
            height: hotnessText.height
            min: 0
            max: 100
            minLabel: "Less"
            maxLabel: "More"
            showFloatingLabel: false
            lowerSliderPos: echonestStation.minHotttness * 100
            upperSliderPos: echonestStation.maxHotttness * 100
            onValueChanged: echonestStation.setHotttness( 1.0 * lowerSliderPos / 100, 1.0 * upperSliderPos / 100 )
        }
    }


    Button {
        id: configureButton
        onClicked: fineTuneView.done();
        text: "configure"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
    }

}
