import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    id: root
    anchors.fill: parent

    property bool showSearchField: true

    property int spacing: defaultFontHeight / 2

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#615858" }
        GradientStop { position: 1.0; color: "#231F1F" }
    }

    Row {
        anchors.fill: parent
        anchors.margins: root.spacing
        spacing: root.spacing

        Image {
            id: iconImage
            source: iconSource
            height: parent.height * 0.8
            width: height
            anchors.verticalCenter: parent.verticalCenter
        }

        Column {
            height: parent.height
            width: parent.width - iconImage.width - toggleViewButtons.width - searchField.width - parent.spacing * 5

            Item {
                id: titleItem
                height: captionText1.height
                width: parent.width
                clip: true

                property string titleText: caption

                onTitleTextChanged: {
                    if(captionText1.text.length > 0) {
                        captionText2.text = caption;
                        renewTitleAnimation.start();
                    } else {
                        captionText1.text = titleText;
                    }
                }

                ParallelAnimation {
                    id: renewTitleAnimation
                    property int duration: 500
                    property variant easingType: Easing.OutBounce;

                    NumberAnimation { target: captionText2; property: "anchors.topMargin"; to: 0; duration: renewTitleAnimation.duration; easing.type: renewTitleAnimation.easingType }
                    NumberAnimation { target: captionText1; property: "anchors.topMargin"; to: captionText1.height * 2; duration: renewTitleAnimation.duration; easing.type: renewTitleAnimation.easingType }

                    onCompleted: {
                        captionText1.text = titleItem.titleText
                        captionText2.anchors.topMargin = -captionText2.height * 2
                        captionText1.anchors.topMargin = 0
                    }
                }

                Text {
                    id: captionText1
                    color: "white"
                    anchors.left: parent.left
                    anchors.top: parent.top

                    font.pointSize: defaultFontSize + 4
                    font.bold: true
                    width: parent.width
                    elide: Text.ElideRight
                }
                Text {
                    id: captionText2
                    color: "white"
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: -height * 2
                    font.pointSize: defaultFontSize + 4
                    font.bold: true
                    width: parent.width
                    elide: Text.ElideRight
                }

            }
            Text {
                text: description
                color: "white"
                font.pointSize:  defaultFontSize + 1
                width: parent.width
                elide: Text.ElideRight
            }
        }

        ToggleViewButtons {
            id: toggleViewButtons
            anchors.verticalCenter: parent.verticalCenter
            height: defaultFontHeight * 1.5
            model: toggleViewButtonModel

            onCurrentIndexChanged: mainView.viewModeSelected(currentIndex)
        }


        SearchField {
            id: searchField
            opacity: root.showSearchField ? 1 : 0
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                rightMargin: root.spacing
            }

            onTextChanged: mainView.setFilterText(text)

        }
    }
}
