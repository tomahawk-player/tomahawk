import QtQuick 1.1

Item {
    id: root
    width: 500
    height: 10

    property int min: 0
    property int max: 100

    /** The labels next to the slider
      * if empty, min and max values are used
      */
    property string minLabel: ""
    property string maxLabel: ""

    /** Should the floating label indicating the current position be shown? */
    property bool showFloatingLabel: true

    property int lowerSliderPos: 25
    property int upperSliderPos: 75

    signal valueChanged()

    Row {
        anchors.fill: parent
        spacing: 10

        Text {
            id: minText
            text: root.minLabel.length > 0 ? root.minLabel : min
            color: "white"
        }

        Item {
            id: sliderRect
            height: root.height
            width: parent.width - minText.width - maxText.width - parent.spacing * 2

            function sliderPosToValue( sliderPos ) {
                var percent = sliderPos * 100 / (sliderRect.width - lowerSlider.width);
                return Math.floor(percent * (root.max - root.min) / 100) + root.min
            }

            function valueToSloderPos( value ) {
                var percent = (value - root.min) * 100 / (root.max - root.min)
                return percent * (sliderRect.width - lowerSlider.width) / 100
            }

            Rectangle {
                id: sliderBase
                height: root.height / 5
                width: parent.width
                color: "white"
                radius: height / 2
                anchors.centerIn: parent

            }
            Rectangle {
                id: lowerSlider
                height: root.height
                width: height
                anchors.top: root.top
                radius: height/2
                border.color: "black"
                border.width: 2
                x: sliderRect.valueToSloderPos(root.lowerSliderPos)

                Rectangle {
                    id: lowerFloatingRect
                    color: "white"
                    anchors.bottom: lowerSlider.top
                    anchors.bottomMargin: 10
                    visible: root.showFloatingLabel && lowerSliderMouseArea.pressed
                    width: lowerFloatingText.width * 1.2
                    height: lowerFloatingText.height + height * 1.2
                    x: -(width - lowerSlider.width) / 2
                    radius: height / 4

                    Text {
                        id: lowerFloatingText
                        anchors.centerIn: parent
                        text: sliderRect.sliderPosToValue(lowerSlider.x)
                    }
                }
            }
            MouseArea {
                id: lowerSliderMouseArea
                anchors.fill: lowerSlider
                drag.target: lowerSlider
                drag.axis: "XAxis"
                drag.minimumX: 0
                drag.maximumX: upperSlider.x - lowerSlider.width
                onReleased: {
                    root.lowerSliderPos = sliderRect.sliderPosToValue( lowerSlider.x );
                    root.valueChanged();
                }
            }

            Rectangle {
                id: upperSlider
                height: root.height
                width: height
                anchors.top: root.top
                radius: height/2
                border.color: "black"
                border.width: 2
                x: sliderRect.valueToSloderPos(root.upperSliderPos)
                Rectangle {
                    id: upperFloatingRect
                    color: "white"
                    anchors.bottom: upperSlider.top
                    anchors.bottomMargin: 10
                    visible: root.showFloatingLabel && upperSliderMouseArea.pressed
                    width: upperFloatingText.width * 1.2
                    height: upperFloatingText.height + height * 1.2
                    radius: height / 4
                    x: -(width - upperSlider.width) / 2

                    Text {
                        id: upperFloatingText
                        anchors.centerIn: parent
                        text: sliderRect.sliderPosToValue(upperSlider.x)
                    }
                }

            }
            MouseArea {
                id: upperSliderMouseArea
                anchors.fill: upperSlider
                onClicked: print("button pressed")
                drag.target: upperSlider
                drag.axis: "XAxis"
                drag.minimumX: lowerSlider.x + lowerSlider.width
                drag.maximumX: parent.width - upperSlider.width
                onReleased: {
                    root.upperSliderPos = sliderRect.sliderPosToValue( upperSlider.x );
                    root.valueChanged();
                }

            }
        }


        Text {
            id: maxText
            text: root.maxLabel.length > 0 ? root.maxLabel : max
            color: "white"
        }
    }
}
