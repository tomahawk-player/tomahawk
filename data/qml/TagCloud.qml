import QtQuick 1.1
import tomahawk 1.0

Item {
    id: tagCloud

    property variant model: 10

    signal tagClicked( string item )

    function randomNumber(min, max) {
        var date = new Date();
        return (max - min) * Math.random(date.getSeconds()) + min
    }

    Flow {
        anchors.centerIn: parent
        width: parent.width
        spacing: 3

        Repeater {
            id: cloudRepeater
            model: tagCloud.model

            delegate: Item {
                id: cloudItem
                width: delegateText.width * 1.1
                height: delegateText.height
                property double itemScale: Math.random() + .3
                scale: itemScale
                Text {
                    id: delegateText
                    color: "white"
                    //text: controlModel.controlAt( index ).summary
                    text: modelData
                    font.pointSize: 16
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: tagCloud.randomNumber(0, 15)
                }
                MouseArea {
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
