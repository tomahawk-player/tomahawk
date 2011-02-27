#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include <QMenu>

#include "trackproxymodel.h"
#include "trackmodel.h"
#include "trackview.h"
#include "viewpage.h"

#include "dllmacro.h"

class DLLEXPORT CollectionView : public TrackView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit CollectionView( QWidget* parent = 0 );
    ~CollectionView();

    virtual void setModel( TrackModel* model );

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return proxyModel(); }

    virtual QString title() const { return model()->title(); }
    virtual QString description() const { return model()->description(); }

    virtual bool showModes() const { return true; }

    virtual bool jumpToCurrentTrack();

private slots:
    void onCustomContextMenu( const QPoint& pos );
    void onTrackCountChanged( unsigned int tracks );

protected:
    virtual void dragEnterEvent( QDragEnterEvent* event );

private:
    void setupMenus();

    QMenu m_itemMenu;

    QAction* m_playItemAction;
    QAction* m_addItemsToQueueAction;
    QAction* m_addItemsToPlaylistAction;
};

#endif // COLLECTIONVIEW_H
