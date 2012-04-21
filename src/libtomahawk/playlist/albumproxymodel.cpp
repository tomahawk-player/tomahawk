/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "albumproxymodel.h"

#include <QListView>

#include "albumproxymodelplaylistinterface.h"
#include "Artist.h"
#include "albumitem.h"
#include "Query.h"
#include "utils/logger.h"


AlbumProxyModel::AlbumProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( 0 )
{
    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceAlbumModel( 0 );
}


void
AlbumProxyModel::setSourceModel( QAbstractItemModel* sourceModel )
{
    Q_UNUSED( sourceModel );
    qDebug() << "Explicitly use setSourceAlbumModel instead";
    Q_ASSERT( false );
}


void
AlbumProxyModel::setSourceAlbumModel( AlbumModel* sourceModel )
{
    m_model = sourceModel;

    if ( m_model && m_model->metaObject()->indexOfSignal( "trackCountChanged(uint)" ) > -1 )
        connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

    QSortFilterProxyModel::setSourceModel( sourceModel );
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


bool
AlbumProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    AlbumItem* p1 = sourceModel()->itemFromIndex( left );
    AlbumItem* p2 = sourceModel()->itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    if ( p1->album().isNull() || p1->album()->artist().isNull() )
        return true;

    if ( p2->album().isNull() || p2->album()->artist().isNull() )
        return false;

    if ( p1->album()->artist()->name() == p2->album()->artist()->name() )
    {
        return QString::localeAwareCompare( p1->album()->name(), p2->album()->name() ) < 0;
    }

    return QString::localeAwareCompare( p1->album()->artist()->name(), p2->album()->artist()->name() ) < 0;
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


Tomahawk::playlistinterface_ptr
AlbumProxyModel::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::AlbumProxyModelPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
