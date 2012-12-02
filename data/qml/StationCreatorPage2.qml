import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Item {
    id: root

    property int margins: defaultFontHeight * 2
    property alias content: contentLoader.source
    property bool nextEnabled: false

    signal back()
    signal next()

    Loader {
        id: contentLoader
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            bottom: backButton.top
            margins: root.margins
        }
    }

    Connections {
        target: contentLoader.item

        onDone: root.next()
    }

    RoundedButton  {
        id: backButton
        text: "<"
        height: defaultFontHeight * 4
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: root.margins
        }

        onClicked: root.back()
    }

    RoundedButton  {
        id: nextButton
        text: ">"
        height: defaultFontHeight * 4
        visible: root.nextEnabled
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: root.margins
        }

        onClicked: root.next()
    }
}
