import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: echonestStation.configured ? "list" : "configure"

    property int coverSize: 230

    onWidthChanged: {
        print("width changed to", width)
        coverView.model = dynamicModel
    }

    states: [
        State {
            name: "configure"
            PropertyChanges { target: styleCloud; anchors.leftMargin: 0 }
            PropertyChanges { target: configureButton; opacity: 1 }
        },
        State {
            name: "finetune"
            PropertyChanges { target: fineTuneView; anchors.rightMargin: 0 }
            PropertyChanges { target: configureButton; opacity: 1 }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                target: styleCloud
                properties: "anchors.leftMargin"; easing.type: Easing.InOutQuad; duration: 500
            }
            NumberAnimation {
                target: fineTuneView
                properties: "anchors.rightMargin"; easing.type: Easing.InOutQuad; duration: 500
            }
            NumberAnimation {
                target: configureButton
                properties: "opacity"; easing.type: Easing.InOutQuad; duration: 500
            }
        }
    ]


    PathView {
        id: coverView
        anchors.fill: parent

        Component.onCompleted: {
            print("pathview created:", scene.width)
        }

        preferredHighlightBegin: 0.1 // scene.width / 11000
        preferredHighlightEnd: preferredHighlightBegin
        pathItemCount: 5
        //highlightMoveDuration: 500

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        property int pathStartX: width - scene.coverSize
        property int pathStartY: height / 2

        delegate: CoverImage {
            height: scene.coverSize
            width: scene.coverSize

            artistName: model.artistName
            trackName: model.trackName
            artworkId: index

            scale: PathView.itemScale
            itemBrightness: PathView.itemBrightness
            opacity: PathView.itemOpacity
            z: x

            onPlayClicked: echonestStation.playItem( index )
        }

        path: Path {
            startX: coverView.pathStartX
            startY: coverView.pathStartY

            PathAttribute { name: "itemOpacity"; value: 0 }
            PathAttribute { name: "itemBrightness"; value: 0 }
            PathAttribute { name: "itemScale"; value: 1.5 }
            PathLine { x: coverView.pathStartX * 9/10 ; y: coverView.pathStartY * 9/10 }
            PathPercent { value: .1 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 1 }
            PathAttribute { name: "itemScale"; value: 1.0 }
            PathLine { x: coverView.pathStartX * .5; y: coverView.pathStartY * .7}
            PathPercent { value: .4 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 1 }
            PathAttribute { name: "itemScale"; value: 0.6 }
            PathLine { x: coverView.pathStartX * .25 ; y: coverView.pathStartY * .25 }
            PathPercent { value: .75 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: .5 }
            PathAttribute { name: "itemScale"; value: 0.4 }
            PathLine { x: 0; y: 0 }
            PathPercent { value: 1 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "itemBrightness"; value: 0 }
            PathAttribute { name: "itemScale"; value: 0.2 }
        }

        states: [
            State {
                name: "normal"
                PropertyChanges { target: coverView; anchors.rightMargin: 0 }
            },
            State {
                name: "shrinked"
                PropertyChanges { target: coverView; anchors.rightMargin: scene.width / 3 }
            }
        ]

        transitions: [
            Transition {
                NumberAnimation {
                    target: coverView
                    properties: "anchors.rightMargin"; easing.type: Easing.InOutQuad; duration: 500
                }
            }
        ]
    }


    Item {
        id: styleCloud
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        anchors.leftMargin: scene.width
        width: scene.width

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
                model: generator.styles()

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
                        anchors.verticalCenterOffset: styleCloud.randomNumber(0, 15)
                    }
                    MouseArea {
                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: echonestStation.setMainControl(modelData);

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

    Item {
        id: fineTuneView
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
        anchors.rightMargin: -width
        width: scene.width / 2

        property color textColor: "white"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 30
            color: "gray"
            border.width: 2
            border.color: "white"
            radius: 20
        }

        Grid {
            Text {
                color: fineTuneView.textColor
                text: "Name"

            }
            TextInput {
                id: stationName
                //onTextChanged: echonestStation.
            }
        }
    }
    Rectangle {
        id: configureButton
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        color: "gray"
        height: 50
        width: 50
        radius: 25
        //opacity: 0

        Text {
            anchors.centerIn: parent
            text: "configure"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                print("changing scene state to", scene.state)
                if( scene.state === "list" ) {
                    scene.state = "finetune";
                    coverView.state = "shrinked"
                } else {
                    scene.state = "list";
                    coverView.state = "normal"
                }
                print("changed scene state to", scene.state)
            }
        }
    }
}
