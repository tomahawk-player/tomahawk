/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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


#include "QueueItem.h"

#include "utils/ImageRegistry.h"
#include "utils/Logger.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableProxyModel.h"
#include "ViewManager.h"
#include "ViewPage.h"
#include "DropJob.h"

#include <QString>
#include <QIcon>


QueueItem::QueueItem( SourcesModel* model, SourceTreeItem* parent )
    : SourceTreeItem( model, parent, SourcesModel::Queue )
    , m_sortValue( -150 )
{
    m_text = tr( "Queue" );
    m_icon = ImageRegistry::instance()->icon( RESPATH "images/queue.svg" );

    connect( ViewManager::instance()->queue()->view()->trackView()->proxyModel(), SIGNAL( itemCountChanged( uint ) ), SIGNAL( updated() ) );
}


QueueItem::~QueueItem()
{
}


QString
QueueItem::text() const
{
    return m_text;
}


QIcon
QueueItem::icon() const
{
    return m_icon;
}


int
QueueItem::peerSortValue() const
{
    return m_sortValue;
}


void
QueueItem::setSortValue( int value )
{
    m_sortValue = value;
}


int
QueueItem::unlistenedCount() const
{
    return ViewManager::instance()->queue()->view()->trackView()->proxyModel()->rowCount();
}


void
QueueItem::activate()
{
    Tomahawk::ViewPage* page = ViewManager::instance()->showQueuePage();
    model()->linkSourceItemToPage( this, page );
}


bool
QueueItem::willAcceptDrag( const QMimeData* data ) const
{
    return DropJob::acceptsMimeData( data, DropJob::Track );
}


SourceTreeItem::DropTypes
QueueItem::supportedDropTypes( const QMimeData* data ) const
{
    if ( data->hasFormat( "application/tomahawk.mixed" ) )
    {
        // If this is mixed but only queries/results, we can still handle them
        bool mixedQueries = true;

        QByteArray itemData = data->data( "application/tomahawk.mixed" );
        QDataStream stream( &itemData, QIODevice::ReadOnly );
        QString mimeType;
        qlonglong val;

        while ( !stream.atEnd() )
        {
            stream >> mimeType;
            if ( mimeType != "application/tomahawk.query.list" &&
                 mimeType != "application/tomahawk.result.list" )
            {
                mixedQueries = false;
                break;
            }
            stream >> val;
        }

        if ( mixedQueries )
            return (DropTypes)(DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50);
        else
            return DropTypesNone;
    }

    if ( data->hasFormat( "application/tomahawk.query.list" ) )
        return (DropTypes)(DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50);
    else if ( data->hasFormat( "application/tomahawk.result.list" ) )
        return (DropTypes)(DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50);
    else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
        return (DropTypes)(DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50);
    else if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
        return (DropTypes)(DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50);
    else if ( data->hasFormat( "text/plain" ) )
    {
        return DropTypesNone;
    }

    return DropTypesNone;
}


bool
QueueItem::dropMimeData( const QMimeData* data, Qt::DropAction action )
{
    Q_UNUSED( action );

    if ( !DropJob::acceptsMimeData( data, DropJob::Track ) )
        return false;

    DropJob *dj = new DropJob();
    connect( dj, SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ), SLOT( parsedDroppedTracks( QList< Tomahawk::query_ptr > ) ) );

    dj->setDropTypes( DropJob::Track );
    dj->setDropAction( DropJob::Append );
    dj->tracksFromMimeData( data, false, false );

    // TODO can't know if it works or not yet...
    return true;
}


void
QueueItem::parsedDroppedTracks( const QList< Tomahawk::query_ptr >& tracks )
{
    if ( tracks.count() )
    {
        ViewManager::instance()->queue()->view()->trackView()->model()->appendQueries( tracks );
    }
    else
        tDebug() << "ERROR: Could not add empty track list to queue!";
}
