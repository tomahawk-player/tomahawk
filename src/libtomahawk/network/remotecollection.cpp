#include "remotecollection.h"

using namespace Tomahawk;

RemoteCollection::RemoteCollection( source_ptr source, QObject* parent )
    :  DatabaseCollection( source, parent )
{
    qDebug() << Q_FUNC_INFO;
}


// adding/removing is done by dbsyncconnection, and the dbcmd objects that modify
// the database will make us emit the appropriate signals (tracksAdded etc.)
void RemoteCollection::addTracks( const QList<QVariant> &newitems )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( false );
}


void RemoteCollection::removeTracks( const QList<QVariant> &olditems )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( false );
}
