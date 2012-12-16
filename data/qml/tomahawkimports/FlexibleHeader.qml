import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    id: root

    // The icon
    property alias icon: iconImage.source

    // The title
    property alias title: titleItem.titleText

    // The subtitle/description
    property alias subtitle: subtitleText.text

    // The model for the ToggleViewButtons.
    // "modelData" role name holds the iconSource
    // => You can use a QStringList or StandardListModel here
    property alias buttonModel: toggleViewButtons.model

    // The index of the currently selected item
    property alias currentButtonIndex: toggleViewButtons.currentIndex

    // Should we show the searchfield?
    property bool showSearchField: true

    // The SearchFields text
    property alias searchText: searchField.text

    property bool showBackButton: false
    property bool showNextButton: false

    property string backButtonText: "Back"
    property string nextButtonText: "Next"

    // Layout spacing
    property int spacing: defaultFontHeight * 0.5

    signal backPressed()
    signal nextPressed()
    signal savePressed()

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#615858" }
        GradientStop { position: 1.0; color: "#231F1F" }
    }

    Row {
        id: leftRow
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: rightRow.left
        }

        anchors.margins: root.spacing
        spacing: root.spacing

        Image {
            id: iconImage
            height: parent.height * 0.8
            width: height
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
        }

        Column {
            height: parent.height
            width: parent.width - iconImage.width - parent.spacing

            Item {
                id: titleItem
                height: captionText1.height
                width: parent.width
                clip: true

                property string titleText

                onTitleTextChanged: {
                    if(captionText1.text.length > 0) {
                        captionText2.text = titleText;
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

                    font.pointSize: defaultFontSize * 1.5
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
                    font.pointSize: defaultFontSize * 1.5
                    font.bold: true
                    width: parent.width
                    elide: Text.ElideRight
                }

            }
            Text {
                id: subtitleText
                color: "white"
                font.pointSize:  defaultFontSize * 1.2
                width: parent.width
                elide: Text.ElideRight
            }
        }

    }

    Row {
        id: rightRow
        anchors {
            top: parent.top
            right: parent.right
            rightMargin: -backButton.width - root.spacing - nextButton.width
            bottom: parent.bottom
            margins: root.spacing
        }
        width: childrenRect.width
        spacing: root.spacing
        layoutDirection: Qt.RightToLeft

        states: [
            State {
                name: "oneVisible"; when: root.showBackButton && !root.showNextButton
                PropertyChanges {
                    target: rightRow
                    anchors.rightMargin: -nextButton.width
                }
            },
            State {
                name: "bothVisible"; when: root.showBackButton && root.showNextButton
                PropertyChanges {
                    target: rightRow
                    anchors.rightMargin: root.spacing
                }
            }

        ]

        Behavior on anchors.rightMargin {
            NumberAnimation { duration: 200 }
        }

        PushButton {
            id: nextButton
            anchors.verticalCenter: parent.verticalCenter
            text: root.nextButtonText
            onClicked: root.nextPressed();
        }
        PushButton {
            id: backButton
            anchors.verticalCenter: parent.verticalCenter
            text: root.backButtonText
            onClicked: root.backPressed();
        }
        InputField {
            id: searchField
            visible: root.showSearchField
            anchors.verticalCenter: parent.verticalCenter
            placeholderText: "Search..."
            showSearchIcon: true
        }
        ToggleViewButtons {
            id: toggleViewButtons
            anchors.verticalCenter: parent.verticalCenter
            height: defaultFontHeight * 1.5
        }
    }
}
