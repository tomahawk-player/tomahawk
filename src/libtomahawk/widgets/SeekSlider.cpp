/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2016, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include "utils/TomahawkStyle.h"
#include "utils/Logger.h"


SeekSlider::SeekSlider( QWidget* parent )
    : QSlider( parent )
    , TomahawkUtils::DpiScaler( this )
    , m_timeLine( 0 )
    , m_acceptWheelEvents( true )
    , m_isScrubbing( false )
{
    setStyleSheet( QString(
                   "QSlider::groove:horizontal {"
                   "margin-top: %1px; margin-bottom: %1px; border: %2px solid rgba(200, 200, 200, 0); background: rgba(200, 200, 200, 40);"
//                   "border-image: url(" RESPATH "images/seek-slider-bkg.png) %2 %2 %2 %2 stretch stretch;"
                   "}"

                   "QSlider::sub-page:horizontal {"
                   "margin-top: %1px; margin-bottom: %1px; border: %2px solid rgba(0, 0, 0, 0); background: %3;"
//                   "border-image: url(" RESPATH "images/seek-slider-level.png) %2 %2 %2 %2 stretch stretch;"
                   "}" )
                   .arg( scaledX( 7 ) /*margin*/ )
                   .arg( 0 /*border*/ )
                   .arg( /*color*/ TomahawkStyle::SEEKSLIDER_FOREGROUND.name() ) +
                   QString(
                   "QSlider::handle:horizontal {"
                   "margin-bottom: -%1px; margin-top: -%1px;"
                   "margin-left: -%2px; margin-right: -%2px;"
                   "height: %3px; width: %4px;"
//                   "background-image: url(" RESPATH "images/seek-and-volume-knob-rest.png);"
                   "background-repeat: no-repeat;"
                   "}" )
                   .arg( /*margin top&bottom*/ 0 )
                   .arg( /*margin left&right*/ 0 )
                   .arg( /*height*/ 0 )
                   .arg( /*width*/ 0 ) );
}


SeekSlider::~SeekSlider()
{
}


void
SeekSlider::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        m_isScrubbing = true;

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


void
SeekSlider::wheelEvent( QWheelEvent* event )
{
    if ( m_acceptWheelEvents )
    {
        QAbstractSlider::wheelEvent( event );
        return;
    }
    event->ignore();
}


void
SeekSlider::mouseMoveEvent( QMouseEvent* event )
{
    if ( !m_isScrubbing )
        return;

    // disable further scrubbing when we're past the slider's right margin
    if ( event->pos().x() > width() )
    {
        m_isScrubbing = false;
        return;
    }

    QSlider::mouseMoveEvent( event );
}
