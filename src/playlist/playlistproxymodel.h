#ifndef PLAYLISTPROXYMODEL_H
#define PLAYLISTPROXYMODEL_H

#include "trackproxymodel.h"

class PlaylistProxyModel : public TrackProxyModel
{
Q_OBJECT

public:
    explicit PlaylistProxyModel( QObject* parent = 0 );
};

#endif // PLAYLISTPROXYMODEL_H
