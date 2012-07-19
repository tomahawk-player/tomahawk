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
            text: "Tempo:"
            color: "white"
        }
        DoubleSlider {
            width: 500
        }

        Text {
            text: "Hotness:"
            color: "white"
        }
        DoubleSlider {
            width: 500
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
