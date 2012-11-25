import QtQuick 1.1

Item {
    id: root

    // Should the artist + track labels be painted
    property bool showLabels: true

    // Should the play button be painted on mouse hover?
    property bool showPlayButton: false

    // if this is true, the play button will be swapped by a pause button
    property bool currentlyPlaying: false

    // Should the mirror be painted?
    property bool showMirror: false

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

    // will be emitted when the on hower play button is clicked
    signal playClicked()
    // will be emitted when the cover is clicked
    signal clicked()
    // will be emitted when the cover is hovered by the mouse
    property alias containsMouse: mouseArea.containsMouse

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            print("Cover clicked");
            root.clicked();
        }
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
                id: itemGlow
                color: "white"
                anchors.fill: parent

                opacity: mouseArea.containsMouse ? .2 : 0

                Behavior on opacity {
                    NumberAnimation { easing.type: Easing.Linear; duration: 300 }
                }
            }

            Rectangle {
                id: textBackground
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                anchors.margins: -1
                height: (artistText.height + trackText.height) * 3
                opacity: showLabels ? 1 : 0
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 0.6; color: "#E1000000" }
                    GradientStop { position: 1.0; color: "#E1000000" }
                }
            }

            Text {
                id: trackText
                color: "white"
                font.bold: true
                text: trackName
                anchors { left: parent.left; right: parent.right; bottom: artistText.top }
                anchors.margins: 2
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                opacity: showLabels ? 1 : 0
                font.pixelSize: root.height / 15
            }
            Text {
                id: artistText
                color: "white"
                font.bold: trackText.text.length == 0
                text: artistName
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                anchors.margins: root.height / 20
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                opacity: showLabels ? 1 : 0
                font.pixelSize: trackText.text.length == 0 ? root.height / 10 : root.height / 15
            }
        }

    }
    Loader {
        sourceComponent: coverImage
        anchors.fill: parent
    }

    Loader {
        id: mirroredCover
        sourceComponent: parent.showMirror ? coverImage : undefined
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

        Behavior on opacity {
            NumberAnimation { easing.type: Easing.Linear; duration: 300 }
        }
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
            // For now the Qt.rgba needs to be manually updated to match the backgroundColor 454e59
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 1-mirrorBrightness) }
            GradientStop { position: 0.5; color: backgroundColor }
        }
    }

    Image {
        id: playButton
        visible: showPlayButton ? (mouseArea.containsMouse || currentlyPlaying) : false
        source: currentlyPlaying ? "../images/pause-rest.png" : "../images/play-rest.png"
        anchors.centerIn: parent
        height: mirroredCover.height / 5
        width: height
        MouseArea {
            anchors.fill: parent
            onClicked: {
                print("Play button clicked");
                root.playClicked();
            }
        }
    }

}
