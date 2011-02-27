#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QMenu>

#include "playlist/trackproxymodel.h"
#include "playlist/playlistmodel.h"
#include "trackview.h"
#include "viewpage.h"

#include "dllmacro.h"

class PlaylistModel;

class DLLEXPORT PlaylistView : public TrackView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();

    PlaylistModel* playlistModel() const { return m_model; }
    virtual void setModel( PlaylistModel* model );

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return proxyModel(); }
    
    virtual QString title() const { return playlistModel()->title(); }
    virtual QString description() const { return m_model->description(); }

    virtual bool jumpToCurrentTrack();

protected:
    void keyPressEvent( QKeyEvent* event );

private slots:
    void onCustomContextMenu( const QPoint& pos );
    void onTrackCountChanged( unsigned int tracks );

    void addItemsToPlaylist();
    void deleteItems();

private:
    void setupMenus();

    PlaylistModel* m_model;

    QMenu m_itemMenu;

    QAction* m_playItemAction;
    QAction* m_addItemsToQueueAction;
    QAction* m_addItemsToPlaylistAction;
    QAction* m_deleteItemsAction;
};

#endif // PLAYLISTVIEW_H
