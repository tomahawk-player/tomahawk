import QtQuick 1.1

Item {
    id: scrollBar
    width: defaultFontHeight / 2

    // the ListView where to attach this scrollbar
    property variant listView
    // the orientation of the scrollbar
    property variant orientation : Qt.Vertical

    property int margin: defaultFontHeight * 0.25

    property color color: "white"

    states: [
        State {
            name: "hidden"; when: !listView.moving
            PropertyChanges { target: scrollBar; opacity: 0 }
        },
        State {
            name: "visible"; when: listView.moving
            PropertyChanges { target: scrollBar; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            from: "hidden"
            to: "visible"
            NumberAnimation { properties: "opacity"; duration: 200 }
        },
        Transition {
            from: "visible"
            to: "hidden"
            NumberAnimation { properties: "opacity"; duration: 2000 }
        }
    ]

    anchors {
        left: orientation == Qt.Vertical ? listView.right : listView.left
        leftMargin: orientation == Qt.Vertical ? scrollBar.margin : 0
        top: orientation == Qt.Vertical ? listView.top : listView.bottom
        topMargin: orientation == Qt.Vertical ? 0 : scrollBar.margin
        bottom: orientation == Qt.Vertical ? listView.bottom : undefined
        right: orientation == Qt.Vertical ? undefined : listView.right
    }

    // A light, semi-transparent background
    Rectangle {
        id: background
        anchors.fill: parent
        radius: orientation == Qt.Vertical ? (width/2 - 1) : (height/2 - 1)
        color: scrollBar.color
        opacity: 0.2
        clip: true
        // Size the bar to the required size, depending upon the orientation.
        Rectangle {
            property real position: orientation == Qt.Vertical ? (listView.contentY / listView.contentHeight) : (listView.contentX / listView.contentWidth)
            property real pageSize: orientation == Qt.Vertical ? (listView.height / listView.contentHeight) : (listView.width / listView.contentWidth)

            x: orientation == Qt.Vertical ? 1 : (position * (scrollBar.width-2) + 1)
            y: orientation == Qt.Vertical ? (position * (scrollBar.height-2) + 1) : 1
            width: orientation == Qt.Vertical ? (parent.width-2) : (pageSize * (scrollBar.width-2))
            height: orientation == Qt.Vertical ? (pageSize * (scrollBar.height-2)) : (parent.height-2)
            radius: orientation == Qt.Vertical ? (width/2 - 1) : (height/2 - 1)
            color: scrollBar.color
            opacity: 1
        }
    }

}
