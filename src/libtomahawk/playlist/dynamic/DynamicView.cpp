/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicView.h"
#include <QPainter>
#include <widgets/overlaywidget.h>

using namespace Tomahawk;


DynamicView::DynamicView( QWidget* parent )
    : PlaylistView( parent )
{
    m_showTimer.setInterval( 5000 );
    m_showTimer.setSingleShot( true );
    
    m_fadeOut = new QPropertyAnimation( overlay(), "opacity" );
    m_fadeOut->setDuration( 500 );
    m_fadeOut->setEndValue( 0 );
    
    connect( &m_showTimer, SIGNAL( timeout() ), m_fadeOut, SLOT( start() ) ); 
    
}

DynamicView::~DynamicView()
{

}

void 
DynamicView::showMessageTimeout( const QString& title, const QString& body )
{
    m_title = title;
    m_body = body;
    m_showTimer.start();
}

void 
DynamicView::paintEvent( QPaintEvent* event )
{
    if ( m_showTimer.isActive() || m_fadeOut->state() == QPropertyAnimation::Running )
    {
        QPainter painter( viewport() );
        overlay()->setText( QString( "%1\n%2" ).arg( m_title, m_body ) );
        overlay()->paint( &painter );
    } else {
        PlaylistView::paintEvent( event );
    }
}
