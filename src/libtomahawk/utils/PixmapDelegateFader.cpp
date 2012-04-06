/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "PixmapDelegateFader.h"
#include "tomahawkutilsgui.h"

#include <QPainter>
#include <QPaintEngine>

using namespace Tomahawk;

#define COVER_FADEIN 1000

PixmapDelegateFader::PixmapDelegateFader( const artist_ptr& artist, const QSize& size, bool forceLoad )
    : m_artist( artist )
    , m_size( size )
{
    if ( !m_artist.isNull() )
    {
        connect( m_artist.data(), SIGNAL( coverChanged() ), this, SLOT( artistChanged() ) );
        m_currentReference = m_artist->cover( size, forceLoad );
    }

    init();
}

PixmapDelegateFader::PixmapDelegateFader( const album_ptr& album, const QSize& size, bool forceLoad )
    : m_album( album )
    , m_size( size )
{
    if ( !m_album.isNull() )
    {
        connect( m_album.data(), SIGNAL( coverChanged() ), this, SLOT( albumChanged() ) );
        m_currentReference = m_album->cover( size, forceLoad );
    }

    init();
}


PixmapDelegateFader::PixmapDelegateFader( const query_ptr& track, const QSize& size, bool forceLoad )
    : m_track( track )
    , m_size( size )
{
    if ( !m_track.isNull() )
    {
        connect( m_track.data(), SIGNAL( coverChanged() ), this, SLOT( trackChanged() ) );
        m_currentReference = m_track->cover( size, forceLoad );
    }

    init();
}


PixmapDelegateFader::~PixmapDelegateFader()
{

}


void
PixmapDelegateFader::init()
{
    m_current = QPixmap( m_size );
    m_current.fill( Qt::transparent );

    m_crossfadeTimeline.setDuration( COVER_FADEIN );
    m_crossfadeTimeline.setUpdateInterval( 20 );
    m_crossfadeTimeline.setFrameRange( 0, 1000 );
    m_crossfadeTimeline.setDirection( QTimeLine::Forward );
    connect( &m_crossfadeTimeline, SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
    connect( &m_crossfadeTimeline, SIGNAL( finished() ), this, SLOT( onAnimationFinished() ) );

    if ( m_currentReference.isNull() )
    {
        // No cover loaded yet, use default and don't fade in
        if ( !m_album.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::CoverInCase, m_size );
        else if ( !m_artist.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::CoverInCase, m_size );
        else if ( !m_track.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::CoverInCase, m_size );

        return;
    }

    m_crossfadeTimeline.start();
}


void
PixmapDelegateFader::albumChanged()
{
    if ( m_album.isNull() || m_album->cover( m_size ).isNull() )
        return;

    setPixmap( m_album->cover( m_size ) );
}

void
PixmapDelegateFader::artistChanged()
{
    if ( m_artist.isNull() || m_artist->cover( m_size ).isNull() )
        return;

    setPixmap( m_artist->cover( m_size ) );
}


void
PixmapDelegateFader::trackChanged()
{
    if ( m_track.isNull() || m_track->cover( m_size ).isNull() )
        return;

    setPixmap( m_track->cover( m_size ) );
}


void
PixmapDelegateFader::setPixmap( const QPixmap& pixmap )
{
    if ( pixmap.isNull() )
        return;

    if ( m_crossfadeTimeline.state() == QTimeLine::Running )
    {
        m_pixmapQueue.enqueue( pixmap );
        return;
    }

    m_oldReference = m_currentReference;
    m_currentReference = pixmap;

    m_crossfadeTimeline.start();
}


void
PixmapDelegateFader::onAnimationStep( int step )
{
    const qreal opacity = ((qreal)step / 1000.);
    const qreal oldOpacity =  ( 1000. - step ) / 1000. ;
    m_current.fill( Qt::transparent );

    // Update our pixmap with the new opacity
    QPainter p( &m_current );

    if ( !m_oldReference.isNull() )
    {
        p.setOpacity( oldOpacity );
        p.drawPixmap( 0, 0, m_oldReference );
    }

    Q_ASSERT( !m_currentReference.isNull() );
    if ( !m_currentReference.isNull() ) // Should never be null..
    {
        p.setOpacity( opacity );
        p.drawPixmap( 0, 0, m_currentReference );
    }

    p.end();

    emit repaintRequest();
    /**
     * Avoids using setOpacity that is slow on X11 (turns off graphics-backed painting, forces fallback to
     * software rasterizer.
     *
     * but a bit buggy.
     */
    /*
     const int opacity = ((float)step* / 1000.) * 255;
     const int oldOpacity = 255 - opacity;
    if ( !m_oldReference.isNull() )
    {
        p.setCompositionMode( QPainter::CompositionMode_Source );
        p.drawPixmap( 0, 0, m_oldReference );

        // Reduce the source opacity by the value of the alpha channel
        p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
        qDebug() << Q_FUNC_INFO << "Drawing old pixmap w/ opacity;" << oldOpacity;
        p.fillRect( m_current.rect(), QColor( 0, 0, 0, oldOpacity ) );
    }

    Q_ASSERT( !m_currentReference.isNull() );
    if ( !m_currentReference.isNull() ) // Should never be null..
    {
        QPixmap temp( m_size );
        temp.fill( Qt::transparent );

        QPainter p2( &temp );
        p2.drawPixmap( 0, 0, m_currentReference );

        p2.setCompositionMode( QPainter::CompositionMode_DestinationIn );
        qDebug() << Q_FUNC_INFO << "Drawing NEW pixmap w/ opacity;" << opacity;

        p2.fillRect( temp.rect(), QColor( 0, 0, 0, opacity ) );
        p2.end();

        p.setCompositionMode( QPainter::CompositionMode_Source );
        p.drawPixmap( 0, 0, temp );
    }
    */

}


void
PixmapDelegateFader::onAnimationFinished()
{
    m_oldReference = QPixmap();
    onAnimationStep( 1000 );

    if ( !m_pixmapQueue.isEmpty() )
    {
        setPixmap( m_pixmapQueue.dequeue() );
    }
}



QPixmap
PixmapDelegateFader::currentPixmap() const
{
    return m_current;
}
