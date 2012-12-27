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

    // set this to true if you want to smoothly scale the cover (be aware of performance impacts)
    property bool smooth: false

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

    Rectangle {
        id: itemShadow
        color: backgroundColor
        anchors.fill: parent

        //opacity: 1 - itemBrightness

        Behavior on opacity {
            NumberAnimation { easing.type: Easing.Linear; duration: 300 }
        }
    }

    Component {
        id: coverImage

        Item {
            property bool isMirror: false

            Image {
                anchors.fill: parent
                source: "image://albumart/" + artworkId + (isMirror ? "-mirror" : "") + (showLabels ? "-labels" : "")
                smooth: root.smooth
                opacity: itemBrightness
                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
            }

            Rectangle {
                id: itemGlow
                anchors.fill: parent
                anchors.topMargin: isMirror ? parent.height / 2 : 0

                opacity: (mouseArea.containsMouse ? .2 : 0)

                Gradient {
                    id: glowGradient
                    GradientStop { position: 0.0; color: "white" }
                    GradientStop { position: 0.7; color: "white" }
                    GradientStop { position: 0.8; color: "#00000000" }
                    GradientStop { position: 1.0; color: "#00000000" }
                }
                Gradient {
                    id: mirrorGlowGradient
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 0.5; color: "#00000000" }
                    GradientStop { position: 1.0; color: "#44FFFFFF" }
                }

                states: [
                    State {
                        name: "mirrored"; when: isMirror
                        PropertyChanges {
                            target: itemGlow
                            gradient: mirrorGlowGradient
                        }
                    },
                    State {
                        name: "normal"; when: !isMirror
                        PropertyChanges {
                            target: itemGlow
                            gradient: glowGradient
                        }
                    }
                ]

                Behavior on opacity {
                    NumberAnimation { easing.type: Easing.Linear; duration: 300 }
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
                opacity: showLabels ? itemBrightness * (isMirror ? 0.5 : 1): 0
                font.pixelSize: root.height / 15
                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
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
                opacity: showLabels ? itemBrightness * (isMirror ? 0.5 : 1) : 0
                font.pixelSize: trackText.text.length == 0 ? root.height / 10 : root.height / 15
                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
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
        onLoaded: {
            item.isMirror = true
        }
        transform : [
            Rotation {
                angle: 180; origin.y: root.height
                axis.x: 1; axis.y: 0; axis.z: 0
            }
        ]
    }

    Image {
        id: playButton
        visible: showPlayButton ? (mouseArea.containsMouse || currentlyPlaying) : false
        source: currentlyPlaying ? "../../images/pause-rest.svg" : "../../images/play-rest.svg"
        anchors.centerIn: parent
        height: mirroredCover.height / 5
        width: height
        smooth: root.smooth
        MouseArea {
            anchors.fill: parent
            onClicked: {
                print("Play button clicked");
                root.playClicked();
            }
        }
    }

}
