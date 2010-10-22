#include "collectionview.h"

#include <QDebug>

#include "playlist/collectionproxymodel.h"

using namespace Tomahawk;


CollectionView::CollectionView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new CollectionProxyModel( this ) );
}


CollectionView::~CollectionView()
{
    qDebug() << Q_FUNC_INFO;
}
