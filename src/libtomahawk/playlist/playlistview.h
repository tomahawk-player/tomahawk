#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QMenu>

#include "trackview.h"

#include "dllmacro.h"

class DLLEXPORT PlaylistView : public TrackView
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();

    void setModel( TrackModel* model );

protected:
    void keyPressEvent( QKeyEvent* event );

private slots:
    void onCustomContextMenu( const QPoint& pos );

    void addItemsToPlaylist();
    void deleteItems();

private:
    void setupMenus();

    QMenu m_itemMenu;

    QAction* m_playItemAction;
    QAction* m_addItemsToQueueAction;
    QAction* m_addItemsToPlaylistAction;
    QAction* m_deleteItemsAction;
};

#endif // PLAYLISTVIEW_H
