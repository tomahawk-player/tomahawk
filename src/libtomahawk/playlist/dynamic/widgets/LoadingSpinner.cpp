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

#include "LoadingSpinner.h"

#include "tomahawk/tomahawkapp.h"

#include <QTimeLine>
#include <QPaintEvent>
#include <QPainter>
#include <qmovie.h>
#include <QLabel>

#define ANIM_LENGTH 300

LoadingSpinner::LoadingSpinner( QWidget* parent )
    : QWidget(parent)
    , m_showHide( new QTimeLine )
{
    m_showHide->setDuration( 300 );
    m_showHide->setStartFrame( 0 );
    m_showHide->setEndFrame( 100 );
    m_showHide->setUpdateInterval( 20  );
    connect( m_showHide, SIGNAL( frameChanged( int ) ), this, SLOT( update() ) );
    connect( m_showHide, SIGNAL( finished() ), this, SLOT( hideFinished() ) );
    
    m_anim = new QMovie( RESPATH "/images/loading-animation.gif" );
    
    connect( m_anim, SIGNAL( frameChanged( int ) ), this, SLOT( update() ) );
    
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    hide();
}

LoadingSpinner::~LoadingSpinner()
{

}

void 
LoadingSpinner::fadeIn()
{
    show();
    m_anim->start();
    m_showHide->setDirection( QTimeLine::Forward );
    m_showHide->start();
}

void 
LoadingSpinner::fadeOut()
{
    m_showHide->setDirection( QTimeLine::Backward );
    m_showHide->start();
}

void 
LoadingSpinner::hideFinished()
{
    if( m_showHide->direction() == QTimeLine::Backward ) {
        hide();
        m_anim->stop();
    }
}


QSize 
LoadingSpinner::sizeHint() const
{
    return QSize( 64, 64 );
}

void 
LoadingSpinner::resizeEvent( QResizeEvent* )
{
    reposition();
}

void 
LoadingSpinner::reposition()
{
    if( !parentWidget() )
        return;
    
    int x = ( parentWidget()->width() / 2 ) - ( width() / 2 );
    int y = ( parentWidget()->height() / 2 ) - ( height() / 2 );
    move( x, y );
    resize( 64, 64 );
}


void 
LoadingSpinner::paintEvent( QPaintEvent* ev )
{
    QPainter p( this );
    
//     qDebug() << "FADING" << ( m_showHide->state() == QTimeLine::Running ) << "at frame:" << m_showHide->currentValue();
    if( m_showHide->state() == QTimeLine::Running ) { // showing or hiding
        p.setOpacity( (qreal)m_showHide->currentValue() );
    }
    p.drawPixmap( rect(), m_anim->currentPixmap() );
    
    
}

