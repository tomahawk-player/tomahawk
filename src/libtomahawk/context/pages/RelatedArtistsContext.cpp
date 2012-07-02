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

#include "RelatedArtistsContext.h"

#include <QHeaderView>

#include "playlist/TreeView.h"
#include "playlist/TreeModel.h"
#include "Source.h"

using namespace Tomahawk;


RelatedArtistsContext::RelatedArtistsContext()
    : ContextPage()
{
    m_relatedView = new TreeView();
    m_relatedView->setGuid( "RelatedArtistsContext" );
    m_relatedView->setUpdatesContextView( false );
    m_relatedModel = new TreeModel( m_relatedView );
    m_relatedView->proxyModel()->setStyle( PlayableProxyModel::Large );
    m_relatedView->setTreeModel( m_relatedModel );
    m_relatedView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_relatedView->setSortingEnabled( false );
    m_relatedView->proxyModel()->sort( -1 );

    QPalette pal = m_relatedView->palette();
    pal.setColor( QPalette::Window, QColor( 0, 0, 0, 0 ) );
    m_relatedView->setPalette( pal );

    m_proxy = new QGraphicsProxyWidget();
    m_proxy->setWidget( m_relatedView );
}


RelatedArtistsContext::~RelatedArtistsContext()
{
}


void
RelatedArtistsContext::setArtist( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;
    if ( !m_artist.isNull() && m_artist->name() == artist->name() )
        return;

    if ( !m_artist.isNull() )
    {
        disconnect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), this, SLOT( onSimilarArtistsLoaded() ) );
    }

    m_artist = artist;

    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );

    m_relatedModel->clear();
    onSimilarArtistsLoaded();
}


void
RelatedArtistsContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    setArtist( Artist::get( query->artist(), false ) );
}


void
RelatedArtistsContext::setAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    setArtist( album->artist() );
}


void
RelatedArtistsContext::onSimilarArtistsLoaded()
{
    foreach ( const artist_ptr& artist, m_artist->similarArtists() )
    {
        m_relatedModel->addArtists( artist );
    }
}
