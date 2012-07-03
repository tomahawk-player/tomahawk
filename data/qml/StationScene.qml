import QtQuick 1.1

Rectangle {
    color: "black"
    anchors.fill: parent
//   height: 200
//   width: 200

    PathView {
        id: view
        anchors.fill: parent
        highlight: appHighlight

        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        focus: true
        model: dynamicModel

        delegate: Rectangle {
            height: 200
            width: 200
            color: "blue"
            border.color: "black"
            border.width: 2
        }

        path: Path {
            startX: 0
            startY: 0
//            PathAttribute { name: "iconScale"; value: 0.5 }
            PathQuad { x: 400; y: 150; controlX: 200; controlY: 75 }
//            PathAttribute { name: "iconScale"; value: 1.0 }
//            PathQuad { x: 390; y: 50; controlX: 350; controlY: 200 }
//            PathAttribute { name: "iconScale"; value: 0.5 }
        }
    }
}
