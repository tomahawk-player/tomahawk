import QtQuick 1.1
import tomahawk 1.0

ListView {
    id: root

    property int delegateHeight: defaultFontHeight * 3

    signal itemClicked(int index)

    delegate: Item {
        width: parent.width
        height: root.delegateHeight

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

        Row {
            anchors.fill: parent
            spacing: defaultFontHeight

            CoverImage {
                id: coverImage
                height: parent.height
                width: height
                showLabels: false
                artworkId: model.coverID
            }
            Text {
                text: model.artistName
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - coverImage.width - parent.spacing
                elide: Text.ElideRight
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: root.itemClicked(index)
            hoverEnabled: true
        }
    }
}
