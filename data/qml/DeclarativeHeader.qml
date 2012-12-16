import QtQuick 1.1
import tomahawk 1.0
import "tomahawkimports"

// Only to be used together with DeclarativeHeader C++ class
// If you want to use the header in QML, use FlexibleHeader

Item {
    anchors.fill: parent

    FlexibleHeader {
        anchors.fill: parent
        icon: iconSource
        title: caption
        subtitle: description
        buttonModel: buttonList

        onSearchTextChanged: mainView.setFilterText(searchText)
        onCurrentButtonIndexChanged: mainView.viewModeSelected(currentButtonIndex)
    }
}
