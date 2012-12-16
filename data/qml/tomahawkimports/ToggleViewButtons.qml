import QtQuick 1.1
import tomahawk 1.0

Row {
    id: root
    width: repeater.width

    property alias model: repeater.model
    property int currentIndex: 0

    Repeater {
        id: repeater
        height: root.height
        width: count * height


        delegate: Image {
            height: repeater.height
            width: height

            source: "../../images/view-toggle-" + (index === root.currentIndex ? "active-" : "inactive-" ) + (index === 0 ? "left" : ( index === repeater.count - 1 ? "right" : "centre" )) +  ".svg"
            smooth: true
            Image {
                anchors.fill: parent
                source: "../../images/" + modelData + (index === root.currentIndex ? "-active.svg" : "-inactive.svg")
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: root.currentIndex = index
            }
        }
    }
}
