import QtQuick 1.1
import tomahawk 1.0

ListView {
    id: root

    signal itemClicked(int index)

    delegate: Item {
        width: parent.width
        height: defaultFontHeight * 3

        Rectangle {
            anchors.fill: parent
            radius: defaultFontHeight / 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00FFFFFF" }
                GradientStop { position: 1.0; color: "#55FFFFFF" }
            }
        }

        Row {
            anchors.fill: parent
            spacing: defaultFontHeight

            CoverImage {
                height: parent.height
                width: height
                showLabels: false
                artworkId: model.coverID
            }
            Text {
                text: model.artistName
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.itemClicked(index)
        }
    }
}
