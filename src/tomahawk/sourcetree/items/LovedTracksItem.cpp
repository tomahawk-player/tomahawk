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

#include "viewpages/PlaylistViewPage.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "playlist/TopLovedTracksModel.h"
#include "playlist/TrackItemDelegate.h"

#include "utils/ImageRegistry.h"
#include "utils/Logger.h"

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
        return QString( tr( "Favorites" ) );
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
        PlaylistViewPage* pv = new PlaylistViewPage( ViewManager::instance()->widget() );
        pv->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::LovedPlaylist, TomahawkUtils::Original, QSize( 128, 128 ) ) );

        TopLovedTracksModel* raModel = new TopLovedTracksModel( pv );
        raModel->setTitle( text() );

        TrackItemDelegate* del = new TrackItemDelegate( TrackItemDelegate::LovedTracks, pv->view()->trackView(), pv->view()->trackView()->proxyModel() );
        pv->view()->trackView()->setPlaylistItemDelegate( del );

        pv->view()->trackView()->setEmptyTip( tr( "Sorry, we could not find any of your Favorites!" ) );
        if ( !par )
        {
            raModel->setDescription( tr( "The most loved tracks from all your friends" ) );
            pv->view()->setGuid( QString( "lovedtracks" ) );
        }
        else
        {
            if ( par->source()->isLocal() )
                raModel->setDescription( tr( "All of your loved tracks" ) );
            else
                raModel->setDescription( tr( "All of %1's loved tracks" ).arg( par->source()->friendlyName() ) );

            pv->view()->setGuid( QString( "lovedtracks/%1" ).arg( par->source()->nodeId() ) );
        }

        pv->view()->trackView()->setPlayableModel( raModel );
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
        qry->track()->setLoved( true );
}


bool
LovedTracksItem::isBeingPlayed() const
{
    return m_lovedTracksPage && m_lovedTracksPage->isBeingPlayed();
}
