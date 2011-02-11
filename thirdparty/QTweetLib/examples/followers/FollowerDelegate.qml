import Qt 4.7

Rectangle {
    property string  name: "Twitter name"
    property string  screenName: "Twitter screen name"
    property string  description: "Twitter description"
    property string  avatarUrl

    width: ListView.view.width - 1;
    height: 100
    radius: 10
    border.width: 1
    border.color: "#000000"

    Image {
        id: avatar
        anchors.top: parent.top
        anchors.left: parent.left
        width: 48; height: 48
        anchors.leftMargin: 2
        anchors.topMargin: 2
        source: avatarUrl
    }

    Text {
        id: screenNameText
        text: screenName
        anchors.leftMargin: 2
        anchors.topMargin: 2
        anchors.left: avatar.right
        anchors.top: parent.top
    }

    Text {
        id: nameText
        text: name
        anchors.leftMargin: 2
        anchors.topMargin: 2
        anchors.top: screenNameText.bottom
        anchors.left: avatar.right
    }

    Text {
        id: descriptionText
        text: description
        anchors.rightMargin: 2
        anchors.leftMargin: 2
        anchors.bottomMargin: 2
        anchors.topMargin: 2
        anchors.bottom: parent.bottom
        anchors.top: avatar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: "WordWrap"
    }
}
