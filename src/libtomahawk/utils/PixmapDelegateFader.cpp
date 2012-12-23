/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QPainter>
#include <QBuffer>
#include <QPaintEngine>
#include <QTimer>

#include "Source.h"
#include "TomahawkUtilsGui.h"
#include "Logger.h"

using namespace Tomahawk;

QWeakPointer< TomahawkUtils::SharedTimeLine > PixmapDelegateFader::s_stlInstance = QWeakPointer< TomahawkUtils::SharedTimeLine >();


QWeakPointer< TomahawkUtils::SharedTimeLine >
PixmapDelegateFader::stlInstance()
{
    if ( s_stlInstance.isNull() )
        s_stlInstance = QWeakPointer< TomahawkUtils::SharedTimeLine> ( new TomahawkUtils::SharedTimeLine() );

    return s_stlInstance;
}


PixmapDelegateFader::PixmapDelegateFader( const artist_ptr& artist, const QSize& size, TomahawkUtils::ImageMode mode, bool forceLoad )
    : m_artist( artist )
    , m_size( size )
    , m_mode( mode )
{
    if ( !m_artist.isNull() )
    {
        connect( m_artist.data(), SIGNAL( updated() ), SLOT( artistChanged() ) );
        connect( m_artist.data(), SIGNAL( coverChanged() ), SLOT( artistChanged() ) );

        m_currentReference = TomahawkUtils::createRoundedImage( m_artist->cover( size, forceLoad ), QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );
    }

    init();
}


PixmapDelegateFader::PixmapDelegateFader( const album_ptr& album, const QSize& size, TomahawkUtils::ImageMode mode, bool forceLoad )
    : m_album( album )
    , m_size( size )
    , m_mode( mode )
{
    if ( !m_album.isNull() )
    {
        connect( m_album.data(), SIGNAL( updated() ), SLOT( albumChanged() ) );
        connect( m_album.data(), SIGNAL( coverChanged() ), SLOT( albumChanged() ) );

        m_currentReference = TomahawkUtils::createRoundedImage( m_album->cover( size, forceLoad ), QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );
    }

    init();
}


PixmapDelegateFader::PixmapDelegateFader( const query_ptr& track, const QSize& size, TomahawkUtils::ImageMode mode, bool forceLoad )
    : m_track( track )
    , m_size( size )
    , m_mode( mode )
{
    if ( !m_track.isNull() )
    {
        connect( m_track.data(), SIGNAL( updated() ), SLOT( trackChanged() ) );
        connect( m_track.data(), SIGNAL( resultsChanged() ), SLOT( trackChanged() ) );
        connect( m_track->displayQuery().data(), SIGNAL( updated() ), SLOT( trackChanged() ) );
        connect( m_track->displayQuery().data(), SIGNAL( coverChanged() ), SLOT( trackChanged() ) );

        m_currentReference = TomahawkUtils::createRoundedImage( m_track->displayQuery()->cover( size, forceLoad ), QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );
    }

    init();
}


PixmapDelegateFader::~PixmapDelegateFader()
{
}


void
PixmapDelegateFader::init()
{
    if ( m_currentReference.isNull() )
        m_defaultImage = true;
    else
        m_defaultImage = false;

    m_startFrame = 0;
    m_fadePct = 100;
    m_connectedToStl = false;

    m_current = QPixmap( m_size );
    m_current.fill( Qt::transparent );

    setSize( m_size );
    if ( m_defaultImage )
        return;

    stlInstance().data()->setUpdateInterval( 20 );
    m_startFrame = stlInstance().data()->currentFrame();
    m_connectedToStl = true;
    m_fadePct = 0;
    connect( stlInstance().data(), SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
}


void
PixmapDelegateFader::setSize( const QSize& size )
{
    m_size = size;

    if ( m_defaultImage )
    {
        // No cover loaded yet, use default and don't fade in
        if ( !m_album.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, m_mode, m_size );
        else if ( !m_artist.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, m_mode, m_size );
        else if ( !m_track.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, m_mode, m_size );
    }
    else
    {
        if ( !m_album.isNull() )
            m_currentReference = TomahawkUtils::createRoundedImage( m_album->cover( m_size ), QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );
        else if ( !m_artist.isNull() )
            m_currentReference = TomahawkUtils::createRoundedImage( m_artist->cover( m_size ), QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );
        else if ( !m_track.isNull() )
            m_currentReference = TomahawkUtils::createRoundedImage( m_track->displayQuery()->cover( m_size ), QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );
    }

    emit repaintRequest();
}


void
PixmapDelegateFader::albumChanged()
{
    if ( m_album.isNull() )
        return;

    QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_album->cover( m_size ) ) );
}


void
PixmapDelegateFader::artistChanged()
{
    if ( m_artist.isNull() )
        return;

    QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_artist->cover( m_size ) ) );
}


void
PixmapDelegateFader::trackChanged()
{
    if ( m_track.isNull() )
        return;

    connect( m_track->displayQuery().data(), SIGNAL( updated() ), SLOT( trackChanged() ), Qt::UniqueConnection );
    connect( m_track->displayQuery().data(), SIGNAL( coverChanged() ), SLOT( trackChanged() ), Qt::UniqueConnection );
    QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_track->displayQuery()->cover( m_size ) ) );
}


void
PixmapDelegateFader::setPixmap( const QPixmap& pixmap )
{
    if ( pixmap.isNull() )
        return;

    m_defaultImage = false;
    const qint64 newImageMd5 = pixmap.cacheKey();

    if ( m_oldImageMd5 == newImageMd5 )
        return;
    m_oldImageMd5 = newImageMd5;

    if ( m_connectedToStl )
    {
        m_pixmapQueue.enqueue( pixmap );
        return;
    }

    m_oldReference = m_currentReference;
    m_currentReference = TomahawkUtils::createRoundedImage( pixmap, QSize( 0, 0 ), m_mode == TomahawkUtils::Grid ? 0.00 : 0.20 );

    stlInstance().data()->setUpdateInterval( 20 );
    m_startFrame = stlInstance().data()->currentFrame();
    m_connectedToStl = true;
    m_fadePct = 0;
    connect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
}


void
PixmapDelegateFader::onAnimationStep( int step )
{
    m_fadePct = (float)( step - m_startFrame ) / 10.0;
    if ( m_fadePct > 100.0 )
        m_fadePct = 100.0;

    if ( m_fadePct == 100.0 )
        QTimer::singleShot( 0, this, SLOT( onAnimationFinished() ) );

    const qreal opacity = m_fadePct / 100.0;
    const qreal oldOpacity =  ( 100.0 - m_fadePct ) / 100.0;
    m_current.fill( Qt::transparent );

    // Update our pixmap with the new opacity
    QPainter p( &m_current );

    if ( !m_oldReference.isNull() )
    {
        p.setOpacity( oldOpacity );
        p.drawPixmap( 0, 0, m_oldReference );
    }

    Q_ASSERT( !m_currentReference.isNull() );
    if ( !m_currentReference.isNull() ) // Should never be null...
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

    m_connectedToStl = false;
    disconnect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );

    if ( !m_pixmapQueue.isEmpty() )
        QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_pixmapQueue.dequeue() ) );
}


QPixmap
PixmapDelegateFader::currentPixmap() const
{
    return m_current;
}
