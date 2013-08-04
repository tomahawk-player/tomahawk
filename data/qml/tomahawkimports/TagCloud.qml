import QtQuick 1.1
import tomahawk 1.0

Item {
    id: tagCloud

    property variant model: 10

    signal tagClicked( string tag )

    function randomNumber(min, max) {
        var date = new Date();
        return (max - min) * Math.random(date.getSeconds()) + min
    }

    Flow {
        anchors.centerIn: parent
        width: parent.width
        spacing: defaultFontSize

        Repeater {
            id: cloudRepeater
            model: tagCloud.model

            delegate: Item {
                id: cloudItem
                width: delegateText.width * 1.1
                height: delegateText.height
                property double itemScale: tagCloud.randomNumber(0.5, 1.2)
                scale: itemScale
                Text {
                    id: delegateText
                    color: "gray"
                    //text: controlModel.controlAt( index ).summary
                    text: modelData
                    font.pixelSize: defaultFontHeight * 1.8
                    anchors.verticalCenter: parent.verticalCenter
                    //anchors.verticalCenterOffset: tagCloud.randomNumber(0, 15)

                    states: [
                        State {
                            name: "hovered"; when: cloudItemMouseArea.containsMouse
                            PropertyChanges {
                                target: delegateText
                                color: "white"
                            }
                        }
                    ]
                    transitions: [
                        Transition {
                            from: "*"
                            to: "hovered"
                            ColorAnimation {
                                duration: 200
                            }
                        },
                        Transition {
                            from: "hovered"
                            to: "*"
                            ColorAnimation {
                                duration: 1000
                            }
                        }
                    ]

                }
                MouseArea {
                    id: cloudItemMouseArea
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: tagCloud.tagClicked( modelData )
                }

                Behavior on scale {
                    NumberAnimation { easing: Easing.Linear; duration: 1000 }
                }
            }
        }
    }
}
