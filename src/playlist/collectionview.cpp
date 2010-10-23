#include "collectionview.h"

#include <QDebug>
#include <QDragEnterEvent>

#include "playlist/collectionproxymodel.h"

using namespace Tomahawk;


CollectionView::CollectionView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new CollectionProxyModel( this ) );

    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );

    setDragDropMode( QAbstractItemView::DragOnly );
    setAcceptDrops( false );
}


CollectionView::~CollectionView()
{
    qDebug() << Q_FUNC_INFO;
}


void
CollectionView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    event->ignore();
}

