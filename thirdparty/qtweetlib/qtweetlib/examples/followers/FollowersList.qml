import Qt 4.7

//followersListModel - external

Item {
    ListView {
        id: followersListView
        width: parent.width; height: parent.height
        clip: true
        model: followersListModel
        delegate: FollowerDelegate {
            id: followerDelegate
            name: nameRole
            screenName: screenNameRole
            description: descriptionRole
            avatarUrl: avatarRole
        }
    }
}
