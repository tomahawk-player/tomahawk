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
    property variant completionModel

    property int spacing: defaultFontHeight * 0.2
    signal accepted( string text )

    onFocusChanged: {
        if(focus) {
            textInput.forceActiveFocus();
        }
    }

    Image {
        id: searchIcon
        anchors {
            left: parent.left
            leftMargin:  root.spacing
            verticalCenter:  parent.verticalCenter
        }
        height: parent.height * 0.6
        width: root.showSearchIcon ? height : 1
        opacity: root.showSearchIcon ? 1 : 0
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
            font.pointSize: defaultFontSize

            onAccepted: root.accepted( text );
            onTextChanged: {
                root.text = text;
                realCompletionListModel.clear();
                for (var i in completionModel) {
                    if (completionModel[i].indexOf(text) == 0) {
                        realCompletionListModel.append({modelData: completionModel[i]})
                    }
                }
            }
        }
        Text {
            width: parent.width
            anchors.centerIn: parent
            text: root.text.length === 0 ? root.placeholderText : ""
            color: "lightgray"
            font.pointSize: defaultFontSize
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
        width: (root.showSearchIcon && root.text.length > 0) ? height : 1
        opacity: (root.showSearchIcon && root.text.length > 0) ? 1 : 0
        smooth: true
        source: "../../images/search-box-dismiss-x.svg"

        MouseArea {
            anchors.fill: parent
            onClicked: textInput.text = ""
        }
    }


    Image {
//        source: "../../images/inputfield-border.svg"
        anchors.fill: parent
        anchors.margins: root.radius * 0.1
        clip: true
    }

    Rectangle {
        anchors {
            top: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: Math.min(completionListView.count, 10) * completionListView.delegateHeight
        color: "white"
        ListView {
            id: completionListView
            anchors.fill: parent
            anchors.rightMargin: scrollBar.width + scrollBar.margin
            clip: true
            model: ListModel {
                id: realCompletionListModel
            }

            property int delegateHeight: defaultFontHeight * 1.25
            delegate: Rectangle {
                height: completionListView.delegateHeight
                color: delegateMouseArea.containsMouse ? "lightblue" : "transparent"
                width: parent.width
                Text {
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        margins: defaultFontHeight / 4
                    }
                    text: modelData
                }
                MouseArea {
                    id: delegateMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        textInput.text = modelData
                        realCompletionListModel.clear();
                    }
                }
            }
        }
        ScrollBar {
            id: scrollBar
            listView: completionListView
            color: "black"
            margin: 0
        }
    }
    MouseArea {
        anchors.fill: parent
        anchors.margins: -99999999
        z: -1
        enabled: completionListView.count > 0
        onClicked: {
            realCompletionListModel.clear();
        }
    }
}
