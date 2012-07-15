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
        width: parent.width - 100
        //model: controlModel
        spacing: 3

        Timer {
            interval: 5000
            running: false
            repeat: true

            onTriggered: {
                for(var i = 0; i < cloudRepeater.count; i++) {
                    var item = cloudRepeater.itemAt(i);
                    if(item.itemScale > 0.6) {
                        item.itemScale = Math.random();
                    } else {
                        item.itemScale = Math.random();
                    }
                }
            }
        }

        Repeater {
            id: cloudRepeater
            model: tagCloud.model

            delegate: Item {
                id: cloudItem
                width: delegateText.width * scale
                height: 28
                property double itemScale: Math.random()
                scale: itemScale
                Text {
                    id: delegateText
                    color: "white"
                    //text: controlModel.controlAt( index ).summary
                    text: modelData
                    font.pixelSize: 28
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: tagCloud.randomNumber(0, 15)
                }
                MouseArea {
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: tagCloud.tagClicked( modelData )

                    onMousePositionChanged: {
                        cloudItem.scale = 1;
                        delegateTimer.restart();
                    }
                }
                Timer {
                    id: delegateTimer
                    interval: 3000
                    repeat: false
                    onTriggered: cloudItem.scale = cloudItem.itemScale
                }

                Behavior on scale {
                    NumberAnimation { easing: Easing.Linear; duration: 1000 }
                }
            }
        }
    }
}
