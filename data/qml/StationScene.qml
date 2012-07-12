import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: echonestStation.configured ? "list" : "configure"

    property int coverSize: 230

    states: [
        State {
            name: "configure" //; when: scene.state === "configure"
            PropertyChanges { target: coverView; anchors.leftMargin: scene.width + scene.coverSize + 20 }
            PropertyChanges { target: styleCloud; anchors.leftMargin: 0 }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                target: coverView
                properties: "anchors.leftMargin"; easing.type: Easing.InOutQuad; duration: 500
            }
            NumberAnimation {
                target: styleCloud
                properties: "anchors.leftMargin"; easing.type: Easing.InOutQuad; duration: 500
            }
        }
    ]


    PathView {
        id: coverView
        anchors.fill: parent


        preferredHighlightBegin: 0.07 // scene.width / 11000
        preferredHighlightEnd: preferredHighlightBegin
        pathItemCount: 4
        highlightMoveDuration: 500

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        delegate: CoverImage {
            height: scene.coverSize
            width: scene.coverSize

            scale: PathView.itemScale

            artistName: model.artistName
            trackName: model.trackName
            artworkId: index

            itemBrightness: PathView.itemBrightness
            z: x
        }

        path: Path {
            startX: coverView.width / 2 + 20
            startY: 155

            PathAttribute { name: "itemOpacity"; value: 0 }
            PathAttribute { name: "itemBrightness"; value: 0 }
            PathAttribute { name: "itemScale"; value: 1.5 }
            PathLine { x: coverView.width / 2; y: 150 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 1 }
            PathAttribute { name: "itemScale"; value: 1.0 }
            PathLine { x: coverView.width / 2 - 100; y: 180;}
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 1 }
            PathAttribute { name: "itemScale"; value: 0.6 }
            PathLine { x: 100; y: 100;}
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrighness"; value: 0.4 }
            PathAttribute { name: "itemScale"; value: 0.4 }
        }
    }

    Item {
        id: styleCloud
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        anchors.leftMargin: scene.width
        width: scene.width

        function randomNumber(min, max) {
            var date = new Date();
            return Math.floor((max - min) * Math.random(date.getSeconds())) + min
        }

        Flow {
            anchors.centerIn: parent
            width: parent.width - 100
            //model: controlModel
            spacing: 3
            Repeater {
                model: generator.styles()

                delegate: Item {
                    width: delegateText.width
                    height: 28
                    Text {
                        id: delegateText
                        color: "white"
                        //text: controlModel.controlAt( index ).summary
                        text: modelData
                        font.pixelSize: styleCloud.randomNumber(11, 28)
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: styleCloud.randomNumber(0, 15)
                        MouseArea {
                            anchors.fill: parent
                            onClicked: echonestStation.setMainControl(modelData);
                        }
                    }
                }
            }
        }
    }
}
