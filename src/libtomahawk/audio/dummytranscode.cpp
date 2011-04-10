/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "dummytranscode.h"

#include <QDebug>

DummyTranscode::DummyTranscode()
    : m_init( false )
{
    qDebug() << Q_FUNC_INFO;
}


DummyTranscode::~DummyTranscode()
{
    qDebug() << Q_FUNC_INFO;
}


void
DummyTranscode::processData( const QByteArray &buffer, bool finish )
{
    m_buffer.append( buffer );
//     qDebug() << "DUMMYTRANSCODING:" << buffer.size();

    if( !m_init && m_buffer.size() >= 16364 ) {
        m_init = true;
        emit streamInitialized( 44100, 2 );
    }
}


void
DummyTranscode::onSeek( int seconds )
{
    m_buffer.clear();
}


void
DummyTranscode::clearBuffers()
{
    m_buffer.clear();
    m_init = false;
}

