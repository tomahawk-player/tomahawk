import QtQuick 1.1
import "tomahawkimports"

Rectangle {
width: 1400
height: 900
color: "black"

BusyIndicator {
  anchors.centerIn: parent
  anchors.horizontalCenterOffset: 200
  height: 200
  width: 200
  count: 11
}

Image {
  id: svgSpinner
  source: "../images/loading-animation.svg"
  smooth: true
  height: 200
  width: 200 
  anchors.centerIn: parent
  anchors.horizontalCenterOffset: -200

  Timer {
    running: true
    repeat: true
    interval: 200
    onTriggered: svgSpinner.rotation += 360 / 12
  }
}


}
