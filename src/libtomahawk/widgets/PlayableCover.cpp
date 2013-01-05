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

#include "Artist.h"
#include "Album.h"
#include "ContextMenu.h"
#include "ViewManager.h"
#include "audio/AudioEngine.h"
#include "widgets/ImageButton.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPainter>

using namespace Tomahawk;

PlayableCover::PlayableCover( QWidget* parent )
    : QLabel( parent )
    , m_showText( false )
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

    m_contextMenu = new ContextMenu( this );
    m_contextMenu->setSupportedActions( ContextMenu::ActionQueue | ContextMenu::ActionCopyLink | ContextMenu::ActionStopAfter | ContextMenu::ActionLove | ContextMenu::ActionPage );
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
    m_button->move( contentsRect().center() - QPoint( 22, 23 ) );
}


void
PlayableCover::mouseMoveEvent( QMouseEvent* event )
{
    QLabel::mouseMoveEvent( event );

    foreach ( const QRect& rect, m_itemRects )
    {
        if ( rect.contains( event->pos() ) )
        {
            if ( m_hoveredRect != rect )
            {
                setCursor( Qt::PointingHandCursor );
                m_hoveredRect = rect;
                repaint();
            }
            return;
        }
    }

    if ( !m_hoveredRect.isNull() )
    {
        setCursor( Qt::ArrowCursor );
        m_hoveredRect = QRect();
        repaint();
    }
}


void
PlayableCover::mouseReleaseEvent( QMouseEvent* event )
{
    QLabel::mouseReleaseEvent( event );
    
    foreach ( const QRect& rect, m_itemRects )
    {
        if ( rect.contains( event->pos() ) )
        {
            if ( m_artist )
                ViewManager::instance()->show( m_artist );
            else if ( m_album )
                ViewManager::instance()->show( m_album->artist() );
            else if ( m_query )
                ViewManager::instance()->show( Tomahawk::Artist::get( m_query->artist() ) );
            
            return;
        }
    }
}


void
PlayableCover::contextMenuEvent( QContextMenuEvent* event )
{
    m_contextMenu->clear();

    if ( m_artist )
        m_contextMenu->setArtist( m_artist );
    else if ( m_album )
        m_contextMenu->setAlbum( m_album );
    else
        m_contextMenu->setQuery( m_query );
    
    m_contextMenu->exec( event->globalPos() );
}


void
PlayableCover::setPixmap( const QPixmap& pixmap )
{
    m_pixmap = TomahawkUtils::createRoundedImage( pixmap, size() );
    repaint();
}


void
PlayableCover::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );
    painter.drawPixmap( 0, 0, pixmap() );

    if ( !m_showText )
        return;

    QRect r = contentsRect().adjusted( margin(), margin(), -margin(), -margin() );
    QPixmap buffer( r.size() );
    buffer.fill( Qt::transparent );
    QPainter bufpainter( &buffer );
    bufpainter.setRenderHint( QPainter::Antialiasing );
    
    QTextOption to;
    to.setWrapMode( QTextOption::NoWrap );
    
    QColor c1;
    c1.setRgb( 0, 0, 0 );
    c1.setAlphaF( 0.00 );
    QColor c2;
    c2.setRgb( 0, 0, 0 );
    c2.setAlphaF( 0.88 );

    QString text;
    QFont font = QLabel::font();
    font.setPointSize( TomahawkUtils::defaultFontSize() );
    QFont boldFont = font;
    boldFont.setBold( true );
    boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 5 );

    QString top, bottom;
    if ( m_artist )
    {
        top = m_artist->name();
    }
    else if ( m_album )
    {
        top = m_album->name();
        bottom = m_album->artist()->name();
    }
    else if ( m_query )
    {
        top = m_query->track();
        bottom = m_query->artist();
    }

    int bottomHeight = QFontMetrics( font ).boundingRect( bottom ).height();
    int topHeight = QFontMetrics( boldFont ).boundingRect( top ).height();
    int frameHeight = bottomHeight + topHeight + 4;
    
    QRect gradientRect = r.adjusted( 0, r.height() - frameHeight * 3, 0, 0 );
    QLinearGradient gradient( QPointF( 0, 0 ), QPointF( 0, 1 ) );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0.0, c1 );
    gradient.setColorAt( 0.6, c2 );
    gradient.setColorAt( 1.0, c2 );
    
    bufpainter.save();
    bufpainter.setPen( Qt::transparent );
    bufpainter.setBrush( gradient );
    bufpainter.drawRect( gradientRect );
    bufpainter.restore();
    
    bufpainter.setPen( Qt::white );
    
    QRect textRect = r.adjusted( 8, r.height() - frameHeight - 16, -8, -16 );
    bool oneLiner = false;
    if ( bottom.isEmpty() )
        oneLiner = true;
    
    bufpainter.setFont( boldFont );
    if ( oneLiner )
    {
        bufpainter.save();
        QFont f = bufpainter.font();

        while ( f.pointSizeF() > 9 && bufpainter.fontMetrics().width( top ) > textRect.width() )
        {
            f.setPointSizeF( f.pointSizeF() - 0.2 );
            bufpainter.setFont( f );
        }
            
        to.setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        text = bufpainter.fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - 3 );
        bufpainter.drawText( textRect, text, to );

        bufpainter.restore();
    }
    else
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignTop );
        text = bufpainter.fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - 3 );
        bufpainter.drawText( textRect, text, to );
        
        bufpainter.setFont( font );

        QRect r = textRect;
        r.setTop( r.bottom() - bufpainter.fontMetrics().height() );
        r.adjust( 4, 0, -4, -1 );

        text = bufpainter.fontMetrics().elidedText( bottom, Qt::ElideRight, textRect.width() - 16 );
        int textWidth = bufpainter.fontMetrics().width( text );
        r.adjust( ( r.width() - textWidth ) / 2 - 6, 0, - ( ( r.width() - textWidth ) / 2 - 6 ), 0 );

        m_itemRects.clear();
        m_itemRects << r;

        if ( m_hoveredRect == r )
        {
            TomahawkUtils::drawQueryBackground( &bufpainter, r );
            bufpainter.setPen( TomahawkUtils::Colors::SELECTION_FOREGROUND );
        }
        
        to.setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
        bufpainter.drawText( textRect.adjusted( 5, -1, -5, -1 ), text, to );
    }

    {
        QBrush brush( buffer );
        QPen pen;
        pen.setColor( Qt::transparent );
        pen.setJoinStyle( Qt::RoundJoin );

        float frameWidthPct = 0.20;
        painter.setBrush( brush );
        painter.setPen( pen );
        painter.drawRoundedRect( r, frameWidthPct * 100.0, frameWidthPct * 100.0, Qt::RelativeSize );
    }
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
    repaint();
}


void
PlayableCover::setAlbum( const Tomahawk::album_ptr& album )
{
    m_album = album;
    repaint();
}


void
PlayableCover::setQuery( const Tomahawk::query_ptr& query )
{
    m_query = query;
    repaint();
}


void PlayableCover::setShowText( bool b )
{
    m_showText = b;
    repaint();
}
