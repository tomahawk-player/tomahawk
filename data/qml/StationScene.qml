import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: "list"

    ListModel {
        id: styleModel
        ListElement { modelData: "acoustic" }
        ListElement { modelData: "alternative" }
        ListElement { modelData: "alternative rock" }
        ListElement { modelData: "classic" }
        ListElement { modelData: "folk" }
        ListElement { modelData: "indie" }
        ListElement { modelData: "pop" }
        ListElement { modelData: "rock" }
        ListElement { modelData: "hip-hop" }
        ListElement { modelData: "punk" }
        ListElement { modelData: "grunge" }
        ListElement { modelData: "indie" }
        ListElement { modelData: "electronic" }
        ListElement { modelData: "country" }
        ListElement { modelData: "jazz" }
        ListElement { modelData: "psychodelic" }
        ListElement { modelData: "soundtrack" }
        ListElement { modelData: "reggae" }
        ListElement { modelData: "house" }
        ListElement { modelData: "drum and base" }
    }

    ListModel {
        id: dummyArtistModel
        ListElement { modelData: "Pink Floyd" }
        ListElement { modelData: "Tool" }
        ListElement { modelData: "Cake" }
        ListElement { modelData: "Metallica" }
        ListElement { modelData: "Red Hot Chili Peppers" }
        ListElement { modelData: "Korn" }
        ListElement { modelData: "Prodigy" }
        ListElement { modelData: "Otto Waalkes" }
    }

    VisualItemModel {
        id: stationVisualModel


//        Column {
//            height: scene.height
//            width: scene.width

//            Row {
//                height: scene.height / 2
//                width: scene.width
//                spacing: width * .1

//                Item {
//                    height: parent.height
//                    width: (parent.width - orText.width - parent.spacing * 2 ) * 2 / 3
//                    GridView {
//                        id: gridView
//                        anchors.fill: parent
//                        anchors.margins: cellWidth / 2
//                        model: dummyArtistModel

//                        cellWidth: gridView.width / 4 - 1 // -1 to make sure there is space for 4 items even with rounding error
//                        cellHeight: cellWidth

//                        delegate: Item {
//                            height: gridView.cellHeight * .9
//                            width: height

//                            CoverImage {
//                                artistName: modelData
//                                anchors.fill: parent

//                                onClicked: {
//                                    echonestStation.setMainControl( EchonestStation.StationTypeArtist, modelData );
//                                    stationListView.incrementCurrentIndex();
//                                }
//                            }
//                        }
//                    }
//                }

//            }

//            Row {
//                height: scene.height / 2
//                width: scene.width * .9
//                anchors.horizontalCenter: parent.horizontalCenter
//                spacing: width * .1

//                TagCloud {
//                    height: parent.height
//                    width: (parent.width - orText.width - parent.spacing * 2 ) * 2 / 3
//                    model: styleModel//generator.styles()
//                    opacity: echonestStation.configured ? 0 : 1

//                    onTagClicked: {
//                        echonestStation.setMainControl( EchonestStation.StationTypeStyle, item );
//                        stationListView.incrementCurrentIndex();
//                    }

//                    Behavior on opacity {
//                        NumberAnimation { duration: 300 }
//                    }
//                }
//                Text {
//                    id: orText
//                    text: "or"
//                    color: "white"
//                    anchors.verticalCenter: parent.verticalCenter
//                }
//                InputField {
//                    anchors.verticalCenter: parent.verticalCenter
//                    width: (parent.width - orText.width - parent.spacing * 2 ) * 1 / 3
//                }
//            }
//        }

        StationView {
            coverSize: Math.min(scene.height, scene.width) / 2
            height: scene.height
            width: scene.width

            onConfigure: stationListView.incrementCurrentIndex();
        }


        StationConfig {
            height: scene.height
            width: scene.width

            onDone: stationListView.decrementCurrentIndex();
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
        //highlightMoveDuration: 400

//        currentIndex: 1

//        Component.onCompleted: {
//            currentIndex = 1
//        }
    }
}
