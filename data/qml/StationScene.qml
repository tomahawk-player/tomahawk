import QtQuick 1.1
import tomahawk 1.0

Rectangle {
    id: scene
    color: "black"
    anchors.fill: parent
    state: echonestStation.configured ? "list" : "configure"


    VisualItemModel {
        id: stationVisualModel

        TagCloud {
            height: scene.height
            width: scene.width
            model: generator.styles()
            opacity: echonestStation.configured ? 0 : 1

            onTagClicked: {
                echonestStation.setMainControl( item );
                stationListView.incrementCurrentIndex();
            }

            Behavior on opacity {
                NumberAnimation { duration: 300 }
            }
        }

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
        highlightMoveDuration: 400

        Component.onCompleted: {
            if ( echonestStation.configured ) {
                currentIndex = 1
            }
        }
    }
}
