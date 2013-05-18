import QtQuick 1.1

Item {
    id: root

    property int min: 0
    property int max: 100

    /** The labels next to the slider
      * if empty, min and max values are used
      */
    property string minLabel: ""
    property string maxLabel: ""

    /** Should the floating label indicating the current position be shown? */
    property bool showFloatingLabel: true
    property bool minMaxLabelsVisible: true

    property int lowerSliderPos: 25
    property int upperSliderPos: 75

    onUpperSliderPosChanged: print("fooooooooo", upperSliderPos)

    signal valueChanged()

    QtObject {
        id: priv

        property int steps: root.max - root.min + 1

        property int sliderHeight: root.height
        property int sliderWidth: root.height / 2
    }

    Row {
        anchors.fill: parent
        anchors.topMargin: defaultFontHeight * 1.2
        anchors.bottomMargin: defaultFontHeight * 1.2
        spacing: 10

        Text {
            id: minText
            text: root.minLabel.length > 0 ? root.minLabel : min
            color: "white"
            visible: root.minMaxLabelsVisible
        }

        Item {
            id: sliderRect
            height: root.height
            property int maxWidth: parent.width - (minText.visible ? minText.width : 0) - (maxText.visible ? maxText.width : 0) - parent.spacing * 2
            width: maxWidth - (maxWidth % priv.steps)
            anchors.horizontalCenter: parent.horizontalCenter

            function sliderPosToValue( sliderPos ) {
                var percent = sliderPos * 100 / (sliderRect.width - priv.sliderWidth/2);
                return Math.floor(percent * (priv.steps-1) / 100) + root.min
            }

            function valueToSloderPos( value ) {
                var percent = (value - root.min) * 100 / (priv.steps-1)
                return percent * (sliderRect.width - priv.sliderWidth/2) / 100
            }

            Rectangle {
                id: sliderBase
                height: root.height / 1.5
                width: parent.width + defaultFontHeight * 1.5
                color: "white"
                radius: height / 2
                anchors.centerIn: parent

                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#ffffffff" }
                    GradientStop { position: 1.0; color: "#aaffffff" }
                }

                Rectangle {
                    anchors.fill: sliderBase
                    anchors.leftMargin: lowerSlider.x + priv.sliderWidth
                    anchors.rightMargin: sliderBase.width - upperSlider.x - priv.sliderWidth
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#aa962c26" }
                        GradientStop { position: 1.0; color: "#962c26" }
                    }
                }

                Row {
                    id: stepRow
                    anchors.fill: parent
                    anchors.leftMargin: defaultFontHeight - lineWidth/2
                    anchors.rightMargin: defaultFontHeight - lineWidth/2
                    property int stepCount: root.max - root.min + 1
                    property int lineHeight: height
                    property int lineWidth: lineHeight / 15
                    spacing: (width - (stepCount * lineWidth)) / stepCount

                    Repeater {
                        model: stepRow.stepCount

                        Rectangle {
                            id: marker
                            height: stepRow.lineHeight * (isHighlight ? 1.2 : 1)
                            width: stepRow.lineWidth
                            color: "black"

                            property bool isHighlight: index % 10 === 0

                            gradient: Gradient {
                                GradientStop { position: 0.0; color: marker.isHighlight ? "white" : "black" }
                                GradientStop { position: 1.0; color: marker.isHighlight ? "#aaffffff" : "black" }
                            }

                            Text {
                                text: root.min + index
                                visible: marker.isHighlight
                                anchors.horizontalCenter: marker.horizontalCenter
                                anchors.top: marker.bottom
                                anchors.topMargin: defaultFontHeight / 2
                                color: "white"
                            }

                        }

                    }
                }

            }

            Rectangle {
                id: lowerSlider
                height: priv.sliderHeight
                width: priv.sliderWidth
                anchors.top: root.top
                radius: height/4
                border.color: "black"
                border.width: 2
                x: sliderRect.valueToSloderPos(root.lowerSliderPos) - priv.sliderWidth/2

                Rectangle {
                    id: lowerFloatingRect
                    color: "white"
                    anchors.bottom: lowerSlider.top
                    anchors.bottomMargin: 10
//                    visible: root.showFloatingLabel && lowerSliderMouseArea.pressed
                    width: lowerFloatingText.width * 1.2
                    height: lowerFloatingText.height + height * 1.2
                    x: -(width - priv.sliderWidth) / 2
                    radius: height / 8

                    Text {
                        id: lowerFloatingText
                        anchors.centerIn: parent
                        text: sliderRect.sliderPosToValue(lowerSlider.x + priv.sliderWidth/2)
                    }
                }
            }
            MouseArea {
                id: lowerSliderMouseArea
                anchors.fill: lowerSlider
                drag.target: lowerSlider
                drag.axis: "XAxis"
                drag.minimumX: -priv.sliderWidth / 2
                drag.maximumX: upperSlider.x - priv.sliderWidth
                onReleased: {
                    root.lowerSliderPos = sliderRect.sliderPosToValue( lowerSlider.x + priv.sliderWidth/2 );
                    root.valueChanged();
                }
            }

            Rectangle {
                id: upperSlider
                height: root.height
                width: height / 2
                anchors.top: root.top
                radius: height / 4
                border.color: "black"
                border.width: 2
                x: sliderRect.valueToSloderPos(root.upperSliderPos)
                Rectangle {
                    id: upperFloatingRect
                    color: "white"
                    anchors.bottom: upperSlider.top
                    anchors.bottomMargin: 10
//                    visible: root.showFloatingLabel && upperSliderMouseArea.pressed
                    width: upperFloatingText.width * 1.2
                    height: upperFloatingText.height + height * 1.2
                    radius: height / 4
                    x: -(width - priv.sliderWidth) / 2

                    Text {
                        id: upperFloatingText
                        anchors.centerIn: parent
                        text: sliderRect.sliderPosToValue(upperSlider.x + priv.sliderWidth/2)
                    }
                }

            }
            MouseArea {
                id: upperSliderMouseArea
                anchors.fill: upperSlider
                onClicked: print("button pressed")
                drag.target: upperSlider
                drag.axis: "XAxis"
                drag.minimumX: lowerSlider.x + priv.sliderWidth
                drag.maximumX: parent.width - priv.sliderWidth
                onReleased: {
                    root.upperSliderPos = sliderRect.sliderPosToValue( upperSlider.x + priv.sliderWidth/2 );
                    root.valueChanged();
                }

            }


        }


        Text {
            id: maxText
            text: root.maxLabel.length > 0 ? root.maxLabel : max
            color: "white"
            visible: root.minMaxLabelsVisible
        }
    }
}
