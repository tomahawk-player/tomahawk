import QtQuick 1.1

Rectangle {
    id: root
    color: "white"
    border.color: "black"
    border.width: defaultFontHeight * 0.1
    radius: defaultFontHeight * 0.25

    height: textInput.height * 1.4
    width: 300

    property bool showSearchIcon: false
    property string text: ""
    property string placeholderText: ""

    property int spacing: defaultFontHeight * 0.2
    signal accepted( string text )

    Image {
        id: searchIcon
        anchors {
            left: parent.left
            leftMargin:  root.spacing
            verticalCenter:  parent.verticalCenter
        }
        height: parent.height * 0.6
        width: root.showSearchIcon ? height : 0
        smooth: true
        source: "../../images/search-icon.svg"
    }

    Item {
        id: textItem
        anchors.left: searchIcon.right
        anchors.leftMargin: root.spacing
        anchors.right: clearIcon.right
        anchors.rightMargin: root.spacing
        height: textInput.height
        anchors.verticalCenter: parent.verticalCenter
        TextInput {
            id: textInput
            width: parent.width
            anchors.centerIn: parent
            text: root.text

            onAccepted: root.accepted( text );
            onTextChanged: root.text = text;
        }
        Text {
            width: parent.width
            anchors.centerIn: parent
            text: root.text.length === 0 ? root.placeholderText : ""
            color: "lightgray"
        }
    }

    Image {
        id: clearIcon
        anchors {
            right: parent.right
            rightMargin:  root.spacing
            verticalCenter:  parent.verticalCenter
        }
        height: parent.height * 0.8
        width: (root.showSearchIcon && root.text.length > 0) ? height : 0
        smooth: true
        source: "../../images/search-box-dismiss-x.svg"

        MouseArea {
            anchors.fill: parent
            onClicked: textInput.text = ""
        }
    }


    BorderImage {
        source: "../../images/inputfield-border.svg"
        anchors.fill: parent
        anchors.margins: root.radius * 0.1
        clip: true
        border.left: defaultFontHeight/4; border.top: defaultFontHeight/4
        border.right: defaultFontHeight/4; border.bottom: defaultFontHeight/4
    }
}
