#include "albumproxymodel.h"

#include <QDebug>
#include <QListView>

#include "tomahawk/query.h"
#include "collectionmodel.h"


AlbumProxyModel::AlbumProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , PlaylistInterface( this )
    , m_model( 0 )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceModel( 0 );
}


void
AlbumProxyModel::setSourceModel( AlbumModel* sourceModel )
{
    m_model = sourceModel;

    connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ),
                      SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

    QSortFilterProxyModel::setSourceModel( sourceModel );
}


void
AlbumProxyModel::setFilter( const QString& pattern )
{
    qDebug() << Q_FUNC_INFO;
    setFilterRegExp( pattern );

    emit filterChanged( pattern );
}


bool
AlbumProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( filterRegExp().isEmpty() )
        return true;

    AlbumItem* pi = sourceModel()->itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    const Tomahawk::album_ptr& q = pi->album();

    QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );

    bool found = true;
    foreach( const QString& s, sl )
    {
        if ( !q->name().contains( s, Qt::CaseInsensitive ) && !q->artist()->name().contains( s, Qt::CaseInsensitive ) )
        {
            found = false;
        }
    }

    return found;
}


void
AlbumProxyModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( !sourceModel() )
        return;
    if ( index.column() > 0 )
        return;

    sourceModel()->removeIndex( mapToSource( index ) );
}


void
AlbumProxyModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    if ( !sourceModel() )
        return;

    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


Tomahawk::result_ptr
AlbumProxyModel::siblingItem( int itemsAway )
{
    qDebug() << Q_FUNC_INFO;
    return Tomahawk::result_ptr( 0 );
}
