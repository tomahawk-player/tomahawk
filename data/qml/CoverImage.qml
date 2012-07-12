import QtQuick 1.1

Item {
    id: root

    // Labels & Cover
    property string artistName
    property string trackName
    property string artworkId

    // The border color for the cover image
    property color borderColor: "black"
    // The border width for the cover image
    property int borderWidth: 2

    // needed to adjust the shadow
    property color backgroundColor: "black"

    // sets the brightness for the item and its mirror (1: brightest, 0: darkest)
    property double itemBrightness: 1
    property double mirrorBrightness: .5


    MouseArea {
        anchors.fill: parent
        onClicked: print("cover clicked")
    }


    Component {
        id: coverImage

        Rectangle {
            color: "white"
            border.color: borderColor
            border.width: borderWidth

            Image {
                anchors.fill: parent
                //anchors.margins: borderWidth
                source: "image://albumart/" + artworkId
            }


            Rectangle {
                id: textBackground
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: 32
                anchors.margins: 5
                color: "black"
                opacity: 0.5
                radius: 3

            }
            Text {
                color: "white"
                font.bold: true
                text: trackName
                anchors { left: textBackground.left; right: textBackground.right; top: textBackground.top }
                anchors.margins: 2
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
            Text {
                color: "white"
                text: artistName
                anchors { left: textBackground.left; right: textBackground.right; bottom: textBackground.bottom }
                anchors.margins: 2
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }

    }
    Loader {
        sourceComponent: coverImage
        anchors.fill: parent
    }

    Loader {
        id: mirroredCover
        sourceComponent: coverImage
        opacity: parent.mirrorOpacity
        anchors.fill: parent
        transform : [
            Rotation {
                angle: 180; origin.y: root.height
                axis.x: 1; axis.y: 0; axis.z: 0
            }
        ]
    }

    Rectangle {
        id: itemShadow
        color: backgroundColor
        anchors.fill: parent
        anchors.bottomMargin: - parent.height

        // scaling might be off a pixel... make sure that the shadow is at least as large as the image
        anchors.leftMargin: -2
        anchors.rightMargin: -2
        anchors.topMargin: -2

        opacity: 1 - itemBrightness
    }

    Rectangle {
        id: mirrorShadow
        color: parent.backgroundColor
        height: parent.height + 2
        width: parent.width + 4
        anchors.centerIn: parent
        anchors.verticalCenterOffset: parent.height

        gradient: Gradient {
            // TODO: no clue how to get the RGB component of the container rectangle color
            // For now the Qt.rgba needs to be manually updated to match the backgroundColor
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 1-mirrorBrightness) }
            GradientStop { position: 0.5; color: backgroundColor }
        }
    }
}
