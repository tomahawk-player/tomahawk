#include "queueproxymodel.h"

#include <QDebug>

#include "playlist/playlistmanager.h"

using namespace Tomahawk;


QueueProxyModel::QueueProxyModel( QObject* parent )
    : PlaylistProxyModel( parent )
{
    qDebug() << Q_FUNC_INFO;
}


QueueProxyModel::~QueueProxyModel()
{
}


Tomahawk::result_ptr
QueueProxyModel::siblingItem( int itemsAway )
{
    qDebug() << Q_FUNC_INFO << rowCount( QModelIndex() );

    setCurrentItem( QModelIndex() );
    Tomahawk::result_ptr res = PlaylistProxyModel::siblingItem( itemsAway );

    qDebug() << "new rowcount:" << rowCount( QModelIndex() );
    if ( rowCount( QModelIndex() ) == 1 )
        PlaylistManager::instance()->hideQueue();

    removeIndex( currentItem() );

    return res;
}
