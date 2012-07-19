import QtQuick 1.1

Item {
    width: 500
    height: 10

    property int min: 0
    property int max: 100

    property int lowerSliderPos: 25
    property int upperSliderPos: 75

    Rectangle {
        id: lowerSlider
        height: parent.height
        width: height
        anchors.top: parent.top
        x: 10
    }

    Rectangle {
        id: upperSlider
        height: parent.height
        width: height
        anchors.top: parent.top
        x: 50
    }

    Rectangle {
        height: 4
        color: "white"
        radius: height / 2
        width: parent.width
        anchors.centerIn: parent
    }
}
