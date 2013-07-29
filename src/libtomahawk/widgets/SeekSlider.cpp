/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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
    , TomahawkUtils::DpiScaler( this )
    , m_timeLine( 0 )
{
    setFixedHeight( scaledY( 20 ) );
    setStyleSheet( QString(
                   "QSlider::groove:horizontal {"
                   "margin: %1px; border-width: %2px;"
                   "border-image: url(" RESPATH "images/seek-slider-bkg.png) %2 %2 %2 %2 stretch stretch;"
                   "}"

                   "QSlider::sub-page:horizontal {"
                   "margin: %1px; border-width: %2px;"
                   "border-image: url(" RESPATH "images/seek-slider-level.png) %2 %2 %2 %2 stretch stretch;"
                   "}" )
                   .arg( 5 /*margin*/)
                   .arg( 3 /*border*/) +
                   QString(
                   "QSlider::handle:horizontal {"
                   "margin-bottom: -%1px; margin-top: -%1px;"
                   "margin-left: -%2px; margin-right: -%2px;"
                   "height: %3px; width: %4px;"
                   "background-image: url(" RESPATH "images/seek-and-volume-knob-rest.png);"
                   "background-repeat: no-repeat;"
                   "}" )
                   .arg( /*margin top&bottom*/ 7 )
                   .arg( /*margin left&right*/ 4 )
                   .arg( /*height*/ 17 )
                   .arg( /*width*/ 16 ) );
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
