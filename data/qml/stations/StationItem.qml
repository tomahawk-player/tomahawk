import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: stationItem

    CoverFlip {
        id: coverView
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height
        width: parent.width
        interactive: false

        backgroundColor: scene.color

        model: dynamicModel
        currentIndex: currentlyPlayedIndex

        onItemPlayPauseClicked: {
            mainView.playItem(index)
        }

        onItemClicked: {
            mainView.playItem(index)
        }

        states: [
            State {
                name: "empty"; when: mainView.loading
                PropertyChanges {
                    target: coverView
                    anchors.rightMargin: -coverView.width
                    anchors.topMargin: - coverView.height
                    scale: 0
                }
            }
        ]
        transitions: [
            Transition {
                from: "empty"
                to: "*"
                NumberAnimation {
                    properties: "anchors.topMargin,anchors.rightMargin,scale"
                    duration: 1000
                    easing.type: Easing.OutQuad
                }
            }

        ]
//                Behavior on anchors.topMargin {
//                    NumberAnimation { duration: 500 }
//                }
//                Behavior on anchors.rightMargin {
//                    NumberAnimation { duration: 500 }
//                }
//                Behavior on scale {
//                    NumberAnimation { duration: 500 }
//                }

    }
    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        height: defaultFontHeight * 4
        width: height
//        count: 12

        opacity: mainView.loading ? 1 : 0
        running: mainView.loading
    }

}
