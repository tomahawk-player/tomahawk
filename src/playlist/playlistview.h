#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QMenu>

#include "tomahawk/tomahawkapp.h"
#include "trackview.h"

class PlaylistView : public TrackView
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();

    void setModel( TrackModel* model );

protected:
    virtual void keyPressEvent( QKeyEvent* event );

private slots:
    void onCustomContextMenu( const QPoint& pos );

    void addItemsToPlaylist();
    void deleteItem();

private:
    void setupMenus();

    QMenu m_itemMenu;

    QAction* m_playItemAction;
    QAction* m_addItemsToQueueAction;
    QAction* m_addItemsToPlaylistAction;
    QAction* m_deleteItemAction;
};

#endif // PLAYLISTVIEW_H
