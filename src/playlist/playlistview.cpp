#include "playlistview.h"

#include <QDebug>

#include "playlist/playlistproxymodel.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new PlaylistProxyModel( this ) );
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;
}
