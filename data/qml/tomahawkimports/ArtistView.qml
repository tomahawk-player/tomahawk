import QtQuick 1.1
import tomahawk 1.0

GridView {
    id: root
    signal itemClicked(int index)
    property int spacing

    delegate: Item {
        width: root.cellWidth - root.spacing / 2
        height: root.cellHeight - root.spacing / 2

        Rectangle {
            id: background
            anchors.fill: parent
            radius: defaultFontHeight / 2
            opacity: 0.5
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00FFFFFF" }
                GradientStop { position: 1.0; color: "#AAFFFFFF" }
            }

            states: [
                State {
                    name: "hovered"; when: mouseArea.containsMouse
                    PropertyChanges { target: background; opacity: 1 }
                }
            ]

            transitions: [
                Transition {
                    from: "*"; to: "hovered"
                    NumberAnimation { properties: "opacity"; duration: 100 }
                },
                Transition {
                    from: "hovered"; to: "*"
                    NumberAnimation { properties: "opacity"; duration: 600 }
                }
            ]
        }

        CoverImage {
            id: coverImage
            height: parent.height
            width: height
            showLabels: true
            artworkId: model.coverID
            artistName: model.artistName
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: root.itemClicked(index)
            hoverEnabled: true
        }
    }
}
