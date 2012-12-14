import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: "list"

    ListModel {
        id: modeModel
        ListElement { label: "By Artist"; image: "../images/artist-placeholder-grid.svg"; creatorContent: "stations/CreateByArtist.qml" }
        ListElement { label: "By Genre"; image: "../images/album-placeholder-grid.svg"; creatorContent: "stations/CreateByGenre.qml" }
        ListElement { label: "By Year"; image: "image://albumart/foobar"; creatorContent: "year" }
    }

    Connections {
        target: stationView

        onPagePicked: {
            switch(createBy) {
            case StationWidget.CreateByArtist:
                stationCreator.content = "stations/CreateByArtist.qml";
                break;
            case StationWidget.CreateByGenre:
                stationCreator.content = "stations/CreateByGenre.qml";
                break;
            case StationWidget.CreateByYear:
                stationCreator.content = "stations/CreateByYear.qml";
                break;
            }

            print("########", createBy, stationCreator.content)
            stationListView.currentIndex = page;
        }
    }

    VisualItemModel {
        id: stationVisualModel

        StationCreatorPage1 {
            height: scene.height
            width: scene.width
            model: modeModel

            onItemClicked: {
                stationCreator.content = modeModel.get(index).creatorContent
                stationListView.incrementCurrentIndex()
            }
        }

        StationCreatorPage2 {
            id: stationCreator
            height: scene.height
            width: scene.width

            onNext: stationListView.incrementCurrentIndex()
        }

        StationView {
            height: scene.height
            width: scene.width
//            visible: stationListView.currentIndex == 1

            onBackClicked: stationListView.decrementCurrentIndex()
        }

    }

    ListView {
        id: stationListView
        anchors.fill: parent
        contentHeight: scene.height
        contentWidth: scene.width
        orientation: ListView.Horizontal
        model: stationVisualModel
        interactive: false
        highlightMoveDuration: 300

        onHeightChanged: {
            contentHeight = scene.height
        }
        onWidthChanged: {
            contentWidth = scene.width
        }
    }

    Image {
        id: backButton
        height: defaultFontHeight * 4
        width: height
        opacity: stationListView.currentIndex == 0 ? 0 : 1
        source: "../images/back-rest.svg"
        smooth: true
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: defaultFontHeight * 2
        }
        MouseArea {
            anchors.fill: parent
            onClicked: stationListView.decrementCurrentIndex()
        }
    }

    Image {
        height: defaultFontHeight * 4
        opacity: stationListView.currentIndex != 2 ? 0 : 1
        source: stationListView.currentIndex == 2 ? "../images/list-add.svg" : "../images/skip-rest.svg"
        smooth: true
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: defaultFontHeight * 2
        }
    }

}
