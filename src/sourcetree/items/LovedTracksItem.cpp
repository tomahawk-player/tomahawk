/*
 *    Copyright 2012, Christopher Reichert <creichert07@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "LovedTracksItem.h"

#include "SourceTreeItem.h"
#include "SourceItem.h"

#include "DropJob.h"
#include "ViewManager.h"

#include "playlist/FlexibleView.h"
#include "playlist/TrackView.h"
#include "playlist/LovedTracksModel.h"
#include "playlist/PlaylistLargeItemDelegate.h"

#include "utils/ImageRegistry.h"

using namespace Tomahawk;


LovedTracksItem::LovedTracksItem( SourcesModel* mdl, SourceTreeItem* parent )
    : SourceTreeItem( mdl, parent, SourcesModel::LovedTracksPage )
    , m_lovedTracksPage( 0 )
    , m_sortValue( -150 )
{
}


LovedTracksItem::~LovedTracksItem()
{
}


QString
LovedTracksItem::text() const
{
    SourceItem* par = dynamic_cast< SourceItem* >( parent() );

    if ( !par )
        return QString( tr( "Top Loved Tracks" ) );
    else
        return QString( tr( "Loved Tracks" ) );
}


QIcon
LovedTracksItem::icon() const
{
    return ImageRegistry::instance()->icon( RESPATH "images/loved_playlist.svg" );
}


void
LovedTracksItem::activate()
{
    if ( !m_lovedTracksPage )
    {
        SourceItem* par = dynamic_cast< SourceItem* >( parent() );
        FlexibleView* pv = new FlexibleView( ViewManager::instance()->widget() );
        pv->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::LovedPlaylist, TomahawkUtils::Original, QSize( 128, 128 ) ) );

        LovedTracksModel* raModel = new LovedTracksModel( pv );
        raModel->setTitle( text() );

        PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::LovedTracks, pv->trackView(), pv->trackView()->proxyModel() );
        connect( del, SIGNAL( updateIndex( QModelIndex ) ), pv->trackView(), SLOT( update( QModelIndex ) ) );
        pv->trackView()->setItemDelegate( del );

        pv->setEmptyTip( tr( "Sorry, we could not find any loved tracks!" ) );
        if ( !par )
        {
            raModel->setDescription( tr( "The most loved tracks from all your friends" ) );
            pv->setGuid( QString( "lovedtracks" ) );
        }
        else
        {
            if ( par->source()->isLocal() )
                raModel->setDescription( tr( "All of your loved tracks" ) );
            else
                raModel->setDescription( tr( "All of %1's loved tracks" ).arg( par->source()->friendlyName() ) );

            pv->setGuid( QString( "lovedtracks/%1" ).arg( par->source()->userName() ) );
        }

        pv->setPlayableModel( raModel );
        raModel->setSource( !par ? source_ptr() : par->source() );

        m_lovedTracksPage = pv;
    }

    ViewManager::instance()->show( m_lovedTracksPage );
    model()->linkSourceItemToPage( this, m_lovedTracksPage );
}


bool
LovedTracksItem::willAcceptDrag( const QMimeData* data ) const
{
    return DropJob::acceptsMimeData( data, DropJob::Track );
}


SourceTreeItem::DropTypes
LovedTracksItem::supportedDropTypes( const QMimeData* data ) const
{
    if ( data->hasFormat( "application/tomahawk.result.list" ) ||
         data->hasFormat( "application/tomahawk.query.list" ) )
        return DropTypeThisTrack;

    return DropTypesNone;
}


bool
LovedTracksItem::dropMimeData( const QMimeData* data, Qt::DropAction action )
{
    Q_UNUSED( action );

    QList< Tomahawk::query_ptr > queries;
    if ( !DropJob::acceptsMimeData( data, DropJob::Track ) )
        return false;

    DropJob *dj = new DropJob();
    dj->setDropTypes( DropJob::Track );
    dj->setDropAction( DropJob::Append );

    connect( dj, SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ),
             this, SLOT( loveDroppedTracks( QList< Tomahawk::query_ptr > ) ) );

    dj->tracksFromMimeData( data, false, false );
    return true;
}


int
LovedTracksItem::peerSortValue() const
{
    return m_sortValue;
}


void
LovedTracksItem::setSortValue(int value)
{
    m_sortValue = value;
}


void
LovedTracksItem::loveDroppedTracks( QList< Tomahawk::query_ptr > qrys )
{
    foreach( Tomahawk::query_ptr qry, qrys )
        qry->setLoved( true );
}
