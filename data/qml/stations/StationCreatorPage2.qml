import QtQuick 1.1
import tomahawk 1.0
import "../tomahawkimports"

Item {
    id: root

    property int margins: defaultFontHeight * 2
    property alias content: contentLoader.source

    signal next(string text)

    Loader {
        id: contentLoader
        anchors.fill: parent
        anchors.margins: root.margins
    }

    Connections {
        target: contentLoader.item

        onDone: root.next(text)
    }

}
