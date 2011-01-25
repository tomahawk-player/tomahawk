#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include <QMenu>

#include "trackview.h"

#include "dllmacro.h"

class DLLEXPORT CollectionView : public TrackView
{
Q_OBJECT

public:
    explicit CollectionView( QWidget* parent = 0 );
    ~CollectionView();

    virtual void setModel( TrackModel* model );

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
