import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"


Item {
    id: root
    property alias model: gridView.model
    property int spacing: 10

    signal itemClicked(int index)

    GridView {
        id: gridView
        anchors.centerIn: parent
        width: root.width * 9 / 10
        height: cellHeight

        cellWidth: (width - 1) / 3
        cellHeight: cellWidth * 10 / 16

        delegate: Image {
            width: gridView.cellWidth - root.spacing
            height: gridView.cellHeight - root.spacing
            source: image

            Rectangle {
                id: textBackground
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    right: parent.right
                }
                height: parent.height / 5
                color: "black"
                opacity: .5

            }
            Text {
                anchors.centerIn: textBackground
                text: label
                color: "white"
                font.bold: true
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.itemClicked(index)
            }
        }
    }
}
