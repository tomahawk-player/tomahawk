import QtQuick 1.1
//import tomahawk 1.0

Rectangle {
    id: root
    height: buttonText.height * 1.4
    width: buttonText.width + (spacing * 2)
    radius: defaultFontHeight * 0.25
//    border.width: defaultFontHeight * 0.05
//    border.color: "#a7a7a7"

    color: "white"
/*    gradient: Gradient {
        GradientStop { position: 0.0; color: mouseArea.pressed ? "#040404" : "#fbfbfb" }
        GradientStop { position: 1.0; color: mouseArea.pressed ? "#8e8f8e" : "#787878" }
    }*/

    property int spacing: defaultFontHeight * 0.5
    property alias text: buttonText.text

    signal clicked()

    Text {
        id: buttonText
        anchors.centerIn: root
        font.pointSize: defaultFontSize
        color: mouseArea.pressed ? "grey" : "black"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
