import QtQuick 1.1

Rectangle {
    color: "black"
    anchors.fill: parent
//   height: 200
//   width: 200


    Component {
        id: pathDelegate
        Item {
            width: 100; height: 100
            scale: PathView.iconScale

            // TODO: Use Image provider here
            Image {
                id: originalImage
                width: 80
                height: 80
                source: index % 2 === 0 ? "http://www.muktware.com/sites/default/files/images/applications/tomahawk_icon.png" : "http://cloud.ohloh.net/attachments/53867/tomahawk-icon-64x64_med.png"
            }

            // mirror image - album art and a gradient filled rectangle for darkening
            Item {
                width: originalImage.width; height: originalImage.height
                anchors.horizontalCenter: originalImage.horizontalCenter

                // transform this item (the image and rectangle) to create the
                // mirror image using the values from the Path
                transform : [
                    Rotation {
                        angle: 180; origin.y: originalImage.height
                        axis.x: 1; axis.y: 0; axis.z: 0
                    },
                    Rotation {
                        angle: PathView.rotateY; origin.x: originalImage.width/2
                        axis.x: 0; axis.y: 1; axis.z: 0
                    },
                    Scale {
                        xScale: PathView.scaleArt; yScale: PathView.scaleArt
                        origin.x: originalImage.width/2; origin.y: originalImage.height/2
                    }
                ]

                // mirror image
                Image {
                    width: originalImage.width; height: originalImage.height
                    source: originalImage.source
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // mirror image dimming gradient filled rectangle
                Rectangle {
                    width: originalImage.width+4; height: originalImage.height
                    anchors.horizontalCenter: parent.horizontalCenter
                    gradient: Gradient {
                        // TODO: no clue how to get the RGB component of the container rectangle color
                        GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.4) }
                        GradientStop { position: 0.3; color: reflectionContainer.color }
                    }
                }
            }

            Text {
                anchors { top: myIcon.bottom; horizontalCenter: parent.horizontalCenter }
                text: label
                smooth: true
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: view.currentIndex = index
            }
        }
    }

    PathView {
        id: view
        anchors { left: parent.left; top: parent.top; right: parent.right }
        height: 300
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        focus: true
        model: albumsModel
        delegate: pathDelegate
        pathItemCount: 8
        path: Path {
            startX: 10
            startY: 50
            PathAttribute { name: "iconScale"; value: 0.5 }
            PathQuad { x: view.width/2; y: 150; controlX: 50; controlY: 200 }
            PathAttribute { name: "iconScale"; value: 1.0 }
            PathQuad { x: view.width; y: 50; controlX: view.width; controlY: 200 }
            PathAttribute { name: "iconScale"; value: 0.5 }
        }
    }


    Component {
        id: gridDelegate
        Item {
            width: 100; height: 100
            scale: PathView.iconScale

            // TODO: Use Image provider here
            Image {
                id: originalImage
                width: 80
                height: 80
                source: index % 2 === 0 ? "http://www.muktware.com/sites/default/files/images/applications/tomahawk_icon.png" : "http://cloud.ohloh.net/attachments/53867/tomahawk-icon-64x64_med.png"
            }

            Text {
                anchors { top: myIcon.bottom; horizontalCenter: parent.horizontalCenter }
                text: label
                smooth: true
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: view.currentIndex = index
            }
        }
    }


    GridView {
        id: grid
        anchors { left: parent.left; top: view.bottom; right: parent.right; bottom: parent.bottom }

        model: albumsModel

        delegate: gridDelegate
    }

}
