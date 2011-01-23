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

private slots:
    void onCustomContextMenu( const QPoint& pos );

protected:
    virtual void dragEnterEvent( QDragEnterEvent* event );
    void paintEvent( QPaintEvent* event );

private:
    void setupMenus();

    QMenu m_itemMenu;

    QAction* m_playItemAction;
    QAction* m_addItemsToQueueAction;
    QAction* m_addItemsToPlaylistAction;
};

#endif // COLLECTIONVIEW_H
