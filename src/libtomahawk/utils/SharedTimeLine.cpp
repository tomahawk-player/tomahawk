/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "SharedTimeLine.h"


namespace TomahawkUtils
{

SharedTimeLine::SharedTimeLine()
    : QObject( 0 )
    , m_refcount( 0 )
{
    m_timeline.setCurveShape( QTimeLine::LinearCurve );
    m_timeline.setFrameRange( 0, INT_MAX );
    m_timeline.setDuration( INT_MAX );
    m_timeline.setUpdateInterval( 40 );
    connect( &m_timeline, SIGNAL( frameChanged( int ) ), SIGNAL( frameChanged( int ) ) );
}


void
SharedTimeLine::connectNotify( const char* signal )
{
    if ( signal == QMetaObject::normalizedSignature( SIGNAL( frameChanged( int ) ) ) ) {
        m_refcount++;
        if ( m_timeline.state() != QTimeLine::Running )
            m_timeline.start();
    }
}


void
SharedTimeLine::disconnectNotify( const char* signal )
{
    if ( signal == QMetaObject::normalizedSignature( SIGNAL( frameChanged( int ) ) ) )
    {
        m_refcount--;
        if ( m_timeline.state() == QTimeLine::Running && m_refcount == 0 )
        {
            m_timeline.stop();
            deleteLater();
        }
    }
}

}