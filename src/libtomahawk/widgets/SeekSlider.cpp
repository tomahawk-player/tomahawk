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

#include "SeekSlider.h"

#include <QtGui/QMouseEvent>
#include <QtCore/QTimeLine>

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


SeekSlider::SeekSlider( QWidget* parent )
    : QSlider( parent )
    , m_timeLine( 0 )
{
    setFixedHeight( 20 );
    setStyleSheet( "QSlider::groove:horizontal {"
                   "margin: 5px; border-width: 3px;"
                   "border-image: url(" RESPATH "images/seek-slider-bkg.png) 3 3 3 3 stretch stretch;"
                   "}"

                   "QSlider::sub-page:horizontal {"
                   "margin: 5px; border-width: 3px;"
                   "border-image: url(" RESPATH "images/seek-slider-level.png) 3 3 3 3 stretch stretch;"
                   "}"

                   "QSlider::handle:horizontal {"
                   "margin-bottom: -7px; margin-top: -7px;"
                   "margin-left: -4px; margin-right: -4px;"
                   "height: 17px; width: 16px;"
                   "background-image: url(" RESPATH "images/seek-and-volume-knob-rest.png);"
                   "background-repeat: no-repeat;"
                   "}" );
}


SeekSlider::~SeekSlider()
{
}


void
SeekSlider::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        QMouseEvent eventSwap( QEvent::MouseButtonRelease, event->pos(), event->globalPos(), Qt::MidButton, Qt::MidButton, event->modifiers() );
        QSlider::mousePressEvent( &eventSwap );
    }
    else
        QSlider::mousePressEvent( event );
}


void
SeekSlider::setValue( int value )
{
//    int newVal = qBound( minimum(), value, maximum() );
    
    if ( !m_timeLine || sender() != m_timeLine ) 
    {
        QSlider::setValue( value );
        return;
    }

    blockSignals( true );
    QSlider::setValue( value );
    blockSignals( false );
}
