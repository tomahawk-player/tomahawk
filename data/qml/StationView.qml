import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"
import "stations"
Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: "list"

    FlexibleHeader {
        id: header
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }
        height: defaultFontHeight * 4
        width: parent.width
        icon: "../images/station.svg"
        title: mainView.title
        subtitle: ""//generator.summary
        showSearchField: false
        showBackButton: stationListView.currentIndex > 0
        showNextButton: !mainView.configured && stationListView.currentIndex == 2
        nextButtonText: "Save"
        backButtonText: mainView.configured ? "Configure" : "Back"

        z: 1 //cover albumcovers that may leave their area

        onBackPressed: {
            if(mainView.configured) {
                return;
            }

            inputBubble.opacity = 0
            stationListView.decrementCurrentIndex()
            if(stationListView.currentIndex == 1) {
                subtitle = modeModel.get(stationCreator.modeIndex).headerSubtitle + "..."
            }
            if(stationListView.currentIndex == 0) {
                subtitle = ""
            }
        }
        // In our case the next button is the save button
        onNextPressed: {
            inputBubble.opacity = 1
            saveNameInput.forceActiveFocus();
        }
    }


    ListModel {
        id: modeModel
        ListElement { label: "By Artist"; image: "../../images/station-artist.svg"; creatorContent: "stations/CreateByArtist.qml"; headerSubtitle: "by" }
        ListElement { label: "By Genre"; image: "../../images/station-genre.svg"; creatorContent: "stations/CreateByGenre.qml"; headerSubtitle: "like" }
        ListElement { label: "By Year"; image: "../../images/station-year.svg"; creatorContent: "stations/CreateByYear.qml"; headerSubtitle: "from" }
    }

    VisualItemModel {
        id: stationVisualModel

        StationCreatorPage1 {
            height: scene.height - header.height
            width: scene.width
            model: modeModel

            onItemClicked: {
                stationCreator.modeIndex = index
                stationListView.incrementCurrentIndex()
                header.subtitle = modeModel.get(index).headerSubtitle + "..."
            }
        }

        StationCreatorPage2 {
            id: stationCreator
            height: stationListView.height
            width: stationListView.width

            property int modeIndex

            content: modeModel.get(modeIndex).creatorContent

            onNext: {
                stationListView.incrementCurrentIndex()
                header.subtitle = modeModel.get(modeIndex).headerSubtitle + " " + text
            }
        }

        StationItem {
            id: stationItem
            height: stationListView.height
            width: stationListView.width
        }
    }


    VisualItemModel {
        id: configuredStationVisualModel


        StationItem {
            id: cfgstationItem
            height: stationListView.height
            width: stationListView.width
        }
    }

    ListView {
        id: stationListView
        anchors {
            left: parent.left
            top: header.bottom
            right: parent.right
            bottom: parent.bottom
        }

        contentHeight: height
        contentWidth: width
        orientation: ListView.Horizontal
        //model: mainView.configured ? configuredStationVisualModel : stationVisualModel
        interactive: false
        highlightMoveDuration: 300

        onHeightChanged: {
            contentHeight = scene.height
        }
        onWidthChanged: {
            contentWidth = scene.width
        }

        Component.onCompleted: {
            model = mainView.configured ? configuredStationVisualModel : stationVisualModel
        }
        onModelChanged: print("ccccccccccccc", mainView.configured)
    }

        Rectangle {
            id: inputBubble
            color: "black"
            border.width: 2
            border.color: "white"
            height: defaultFontHeight * 3
            width: height * 6
            radius: defaultFontHeight / 2
            anchors.top: header.bottom
            anchors.right: parent.right
            anchors.rightMargin: defaultFontHeight / 2
            anchors.topMargin: -defaultFontHeight / 2
            z: 2
            opacity: 0
            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }

            Row {
                anchors.centerIn: parent
                width: parent.width - defaultFontHeight
                spacing: defaultFontHeight / 2

                function saveStation(name) {
                    mainView.title = name
                    inputBubble.opacity = 0
                    header.showNextButton = false
                    header.backButtonText = "Configure"
                }

                Text {
                    id: nameText
                    color: "white"
                    text: "Name:"
                    anchors.verticalCenter: parent.verticalCenter
                }
                InputField {
                    id: saveNameInput
                    width: parent.width - nameText.width - saveOkButton.width - parent.spacing * 2
                    placeholderText: "Station"
                    onAccepted: parent.saveStation(text);
                }
                PushButton {
                    id: saveOkButton
                    text: "OK"
                    onClicked: parent.saveStation(saveNameInput.text)
                }
            }

        }

}
