import QtQuick 1.1

Rectangle {
    id: root
    color: "white"
    border.color: "black"
    border.width: 2

    height: textInput.height + 4
    width: 300

    property alias text: textInput.text

    signal accepted( string text )

    TextInput {
        id: textInput
        width: parent.width
        anchors.centerIn: parent

        onAccepted: root.accepted( text );
    }
}
