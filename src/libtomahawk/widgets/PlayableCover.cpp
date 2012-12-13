/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011 - 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "PlayableCover.h"

#include "audio/AudioEngine.h"
#include "widgets/ImageButton.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPainter>


PlayableCover::PlayableCover( QWidget* parent )
    : QLabel( parent )
{
    setMouseTracking( true );

    m_button = new ImageButton( this );
    m_button->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PlayButton, TomahawkUtils::Original, QSize( 48, 48 ) ) );
    m_button->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PlayButtonPressed, TomahawkUtils::Original, QSize( 48, 48 ) ), QIcon::Off, QIcon::Active );
    m_button->setFixedSize( 48, 48 );
    m_button->setContentsMargins( 0, 0, 0, 0 );
    m_button->setFocusPolicy( Qt::NoFocus );
    m_button->installEventFilter( this );
    m_button->hide();

    connect( m_button, SIGNAL( clicked( bool ) ), SLOT( onClicked() ) );
}


PlayableCover::~PlayableCover()
{
}


void
PlayableCover::enterEvent( QEvent* event )
{
    QLabel::enterEvent( event );

    m_button->show();
}


void
PlayableCover::leaveEvent( QEvent* event )
{
    QLabel::leaveEvent( event );

    m_button->hide();
}


void
PlayableCover::resizeEvent( QResizeEvent* event )
{
    QLabel::resizeEvent( event );
    m_button->move( contentsRect().center() - QPoint( 23, 23 ) );
}


void
PlayableCover::onClicked()
{
    if ( m_artist )
        AudioEngine::instance()->playItem( m_artist );
    else if ( m_album )
        AudioEngine::instance()->playItem( m_album );
    else if ( m_query )
        AudioEngine::instance()->playItem( Tomahawk::playlistinterface_ptr(), m_query );
}


void
PlayableCover::setArtist( const Tomahawk::artist_ptr& artist )
{
    m_artist = artist;
}


void
PlayableCover::setAlbum( const Tomahawk::album_ptr& album )
{
    m_album = album;
}


void
PlayableCover::setQuery( const Tomahawk::query_ptr& query )
{
    m_query = query;
}
