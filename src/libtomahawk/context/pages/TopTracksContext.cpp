/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TopTracksContext.h"

#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistView.h"
#include "Source.h"

using namespace Tomahawk;


TopTracksContext::TopTracksContext()
    : ContextPage()
{
    m_topHitsView = new PlaylistView();
    m_topHitsView->setGuid( "TopTracksContext" );
    m_topHitsView->setUpdatesContextView( false );
    m_topHitsModel = new PlaylistModel( m_topHitsView );
    m_topHitsView->proxyModel()->setStyle( PlayableProxyModel::Short );
    m_topHitsView->setPlaylistModel( m_topHitsModel );
    m_topHitsView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    QPalette pal = m_topHitsView->palette();
    pal.setColor( QPalette::Window, QColor( 0, 0, 0, 0 ) );
    m_topHitsView->setPalette( pal );

    m_proxy = new QGraphicsProxyWidget();
    m_proxy->setWidget( m_topHitsView );
}


TopTracksContext::~TopTracksContext()
{
}


void
TopTracksContext::setArtist( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;
    if ( !m_artist.isNull() && m_artist->name() == artist->name() )
        return;

    if ( !m_artist.isNull() )
    {
        disconnect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                    this,              SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );
    }

    m_artist = artist;

    connect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );

    m_topHitsModel->clear();
    onTracksFound( m_artist->tracks(), Mixed );
}


void
TopTracksContext::setAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    setArtist( album->artist() );
}


void
TopTracksContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    setArtist( Artist::get( query->artist(), false ) );
}


void
TopTracksContext::onTracksFound( const QList<Tomahawk::query_ptr>& queries, ModelMode mode )
{
    Q_UNUSED( mode );

    m_topHitsModel->appendQueries( queries );
}


