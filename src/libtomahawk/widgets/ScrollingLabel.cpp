/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Teo Mrnjavac <teo@kde.org>
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

#include "ScrollingLabel.h"

#include "utils/DpiScaler.h"

#include <QPainter>


ScrollingLabel::ScrollingLabel( QWidget* parent)
    : QLabel( parent )
    , m_isMouseOver( false )
    , m_scrollPos( 0 )
{
    m_staticText.setTextFormat( Qt::PlainText );

    m_separator = QString::fromUtf8( "    \u26AB    " );

    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( onTimerTimeout() ) );
    m_timer.setInterval( 10 );
}


void
ScrollingLabel::setText( const QString& text )
{
    QLabel::setText( text );
    updateText();
    update();
}


void
ScrollingLabel::updateText()
{
    m_timer.stop();

    m_singleTextWidth = fontMetrics().width( text() );
    m_scrollEnabled = ( m_singleTextWidth > width() - indent() );

    m_scrollPos = -64;

    if ( m_scrollEnabled && m_isMouseOver )
    {
        m_staticText.setText( text() + m_separator );
        m_timer.start();
    }
    else
        m_staticText.setText( text() );

    m_staticText.prepare( QTransform(), font() );
    m_wholeTextSize = QSize( fontMetrics().width( m_staticText.text() ),
                             fontMetrics().height() );
}


void
ScrollingLabel::paintEvent( QPaintEvent* )
{
    QPainter p( this );

    if ( m_scrollEnabled )
    {
        m_buffer.fill( qRgba(0, 0, 0, 0) );
        QPainter pb( &m_buffer );
        pb.setPen( p.pen() );
        pb.setFont( p.font() );

        int x = qMin( -m_scrollPos, 0 ) + indent();
        while ( x < width() )
        {
            pb.drawStaticText( QPointF( x, ( height() - m_wholeTextSize.height() ) / 2 ) + QPoint( 2, 2 ), m_staticText );
            x += m_wholeTextSize.width();
        }

        TomahawkUtils::DpiScaler s( this );
        //Apply Alpha Channel
        pb.setCompositionMode( QPainter::CompositionMode_DestinationIn );
        pb.setClipRect( width() - s.scaledX( 15 ),
                        0,
                        s.scaledX( 15 ),
                        height() );
        pb.drawImage( 0, 0, m_alphaChannel );
        pb.setClipRect( 0, 0, s.scaledX( 15 ), height() );
        //initial situation: don't apply alpha channel in the left half of the image at all; apply it more and more until scrollPos gets positive
        if ( m_scrollPos < 0 )
            pb.setOpacity( (qreal)( qMax( -s.scaledX( 8 ), m_scrollPos ) + s.scaledX( 8 ) ) / s.scaledX( 8 ) );
        pb.drawImage( 0, 0, m_alphaChannel );

        //pb.end();
        p.drawImage( 0, 0, m_buffer );
    }
    else
    {
        if ( m_wholeTextSize.width() > width() - indent() )
        {
            p.drawStaticText( QPointF( indent(), ( height() - m_wholeTextSize.height() ) / 2 ),
                              m_staticText );
        }
        else
        {
            //Horizontal alignment matters here
            if ( alignment().testFlag( Qt::AlignLeft ) )
            {
                p.drawStaticText( QPointF( indent(), ( height() - m_wholeTextSize.height() ) / 2 ),
                                  m_staticText );
            }
            else if ( alignment().testFlag( Qt::AlignRight ) )
            {
                p.drawStaticText( QPointF( width() - m_wholeTextSize.width(),
                                           ( height() - m_wholeTextSize.height() ) / 2 ),
                                  m_staticText );
            }
            else //center!
            {
                p.drawStaticText( QPointF( ( width() - m_wholeTextSize.width() ) / 2.,
                                           ( height() - m_wholeTextSize.height() ) / 2. ),
                                  m_staticText );
            }
        }
    }
}


void
ScrollingLabel::resizeEvent( QResizeEvent* )
{
    //From limmes@StackOverflow
    //When the widget is resized, we need to update the alpha channel.
    m_alphaChannel = QImage( size(), QImage::Format_ARGB32_Premultiplied );
    m_buffer = QImage( size(), QImage::Format_ARGB32_Premultiplied );

    //Create Alpha Channel:
    if ( width() >  64 )
    {
        //create first scanline
        QRgb* scanline1 = (QRgb*)m_alphaChannel.scanLine( 0 );
        for ( int x = 1; x < 16; ++x )
            scanline1[ x - 1 ] = scanline1[ width() - x ] = qRgba( 0, 0, 0, x << 4 );
        for ( int x = 15; x < width() - 15; ++x )
            scanline1[ x ] = qRgb( 0, 0, 0 );
        //copy scanline to the other ones
        for ( int y = 1; y < height(); ++y )
            memcpy( m_alphaChannel.scanLine( y ), (uchar*)scanline1, width() * 4 );
    }
    else
        m_alphaChannel.fill( qRgb( 0, 0, 0 ) );


    //Update scrolling state
    bool newScrollEnabled = ( m_singleTextWidth > width() - indent() );
    if( newScrollEnabled != m_scrollEnabled )
        updateText();
}


void
ScrollingLabel::enterEvent( QEvent* )
{
    m_isMouseOver = true;
    updateText();
    update();
}


void
ScrollingLabel::leaveEvent( QEvent* )
{
    m_isMouseOver = false;
    updateText();
    update();
}


void
ScrollingLabel::onTimerTimeout()
{
    m_scrollPos = ( m_scrollPos + 1 ) % m_wholeTextSize.width();
    update();
}
