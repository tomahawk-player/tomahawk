import QtQuick 1.1
import tomahawk 1.0

Item {
    id: fineTuneView

    property color textColor: "white"

    signal done();

    Grid {
        anchors.fill: parent
        anchors.margins: 50
        Text {
            color: fineTuneView.textColor
            text: "Name"

        }
        TextInput {
            id: stationName
            width: 200
            text: echonestStation.name
            //onTextChanged: echonestStation.
        }
    }

    Rectangle {
        id: configureButton
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        color: "gray"
        height: 20
        width: 150
        radius: 10
        //opacity: 0

        Text {
            anchors.centerIn: parent
            text: "configure"
            color: "white"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                fineTuneView.done();
            }
        }
    }


}
