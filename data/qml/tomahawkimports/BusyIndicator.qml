import QtQuick 1.1

Item {
    id: busyIndicator
    width: 100
    height: width
    property int barWidth: width / 10
    property int barHeight: height / 4
    property int count: 12
    property color color: "white"
    property int currentHighlight: 0
    property bool running: true
    property int interval: 200

    Behavior on opacity {
        NumberAnimation { duration: 500 }
    }

    Repeater {
        model: busyIndicator.count


        Item {
            height: parent.height
            width: busyIndicator.barWidth
            anchors.centerIn: parent
            Rectangle {
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                height: busyIndicator.barHeight
                radius: width / 2

                color: busyIndicator.color
            }
            rotation: 360 / busyIndicator.count * index
            opacity: 1 - ((index > busyIndicator.currentHighlight ? busyIndicator.currentHighlight + busyIndicator.count : busyIndicator.currentHighlight) - index) / busyIndicator.count
            Behavior on opacity {
                NumberAnimation { duration: busyIndicator.interval }
            }
        }
    }

    Timer {
        interval: busyIndicator.interval
        running: busyIndicator.running
        repeat: true
        onTriggered: parent.currentHighlight = (parent.currentHighlight + 1) % busyIndicator.count
    }
}
