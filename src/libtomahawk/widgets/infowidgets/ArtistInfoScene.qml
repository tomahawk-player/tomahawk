import QtQuick 1.1

Rectangle {
    color: "darkgray"
    anchors.fill: parent
//   height: 200
//   width: 200


    Component {
        id: appDelegate
        Item {
            width: 100; height: 100
            scale: PathView.iconScale

            // Use Image provider here
//            Image {
//                id: myIcon
//                y: 20; anchors.horizontalCenter: parent.horizontalCenter
//                source: icon
//                smooth: true
//            }
            Rectangle {
                width: 80
                height: 80
                color: "blue"
            }

            Text {
                anchors { top: myIcon.bottom; horizontalCenter: parent.horizontalCenter }
                text: label
                smooth: true
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
        height: 200
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        focus: true
        model: albumsModel
        delegate: appDelegate
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

    GridView {
        id: grid
        anchors { left: parent.left; top: view.bottom; right: parent.right; bottom: parent.bottom }

        model: albumsModel

        delegate: appDelegate
    }

}
