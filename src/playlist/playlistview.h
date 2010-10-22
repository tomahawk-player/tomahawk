#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "tomahawk/tomahawkapp.h"
#include "trackview.h"

class PlaylistView : public TrackView
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();
};

#endif // PLAYLISTVIEW_H
