import QtQuick 1.1

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent

    property int coverSize: 230

    Component {
        id: coverImage

        Rectangle {
            height: scene.coverSize
            width: scene.coverSize
            color: scene.color
            border.color: scene.color
            border.width: 2

            property string artistName
            property string trackName
            property string artworkId

            Image {
                anchors.fill: parent
                anchors.margins: parent.border.width
                source: "image://albumart/" + parent.artworkId
                onSourceChanged: print("!*!*!*!*!*!*!*!*!", source)
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

            MouseArea {
                anchors.fill: parent
                onClicked: print("cover clicked")
            }
        }
    }

    Component {
        id: mirroredDelegate

        Item {
            id: mirrorItem
            height: scene.coverSize
            width: scene.coverSize

            z: x
            scale: PathView.itemScale

            property double itemOpacity: PathView.itemOpacity
            property double shadowOp: PathView.shadowOpacity

            Loader {
                id: normalCover
                sourceComponent: coverImage
                opacity: parent.itemOpacity
                onLoaded: {
                    item.trackName = trackName
                    item.artistName = artistName
                    item.artworkId = index
                }
            }
            Loader {
                id: mirroredCover
                sourceComponent: coverImage
                opacity: parent.itemOpacity
                onLoaded: {
                    item.trackName = trackName
                    item.artistName = artistName
                    item.artworkId = index
                }
                transform : [
                    Rotation {
                        angle: 180; origin.y: scene.coverSize
                        axis.x: 1; axis.y: 0; axis.z: 0
                    }
                ]
            }

            Rectangle {
                color: scene.color
                anchors.fill: parent
                opacity: mirrorItem.shadowOp
            }
            Rectangle {
                color: scene.color
                height: scene.coverSize
                width: scene.coverSize
                anchors.centerIn: parent
                anchors.verticalCenterOffset: scene.coverSize
                gradient: Gradient {
                    // TODO: no clue how to get the RGB component of the container rectangle color
                    // For now the Qt.rgba needs to be manually updated to match scene.color
                    GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, mirrorItem.shadowOp + ( (1 - mirrorItem.shadowOp) * .4)) }
                    GradientStop { position: 0.5; color: scene.color }
                }
            }
        }
    }

    PathView {
        id: view
        anchors.fill: parent
        highlight: appHighlight

        preferredHighlightBegin: 0.1
        preferredHighlightEnd: 0.1
        pathItemCount: 3
        highlightMoveDuration: 500

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        delegate: mirroredDelegate


        onCurrentIndexChanged: print("***************************************** current index:", currentIndex)

        path: Path {
            startX: scene.width / 2 + 20
            startY: 155
            PathAttribute { name: "itemOpacity"; value: 0 }
            PathAttribute { name: "shadowOpacity"; value: 0 }
            PathAttribute { name: "itemScale"; value: 1.0 }
            PathLine { x: scene.width / 2; y: 150 }
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "shadowOpacity"; value: 0 }
            PathAttribute { name: "itemScale"; value: 1.0 }
            PathLine { x: 100; y: 100;}
            PathAttribute { name: "itemOpacity"; value: 1 }
            PathAttribute { name: "shadowOpacity"; value: 1 }
            PathAttribute { name: "itemScale"; value: 0.75 }
        }
    }
}
