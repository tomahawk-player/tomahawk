/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Qnr_IoDeviceStream.h"

#include "utils/Logger.h"

#include <QNetworkReply>
#include <QTimer>

using namespace Tomahawk;

// Feed Phonon in 1MiB blocks
#define BLOCK_SIZE 1048576

QNR_IODeviceStream::QNR_IODeviceStream( QNetworkReply* reply, QObject* parent )
    : Phonon::AbstractMediaStream( parent )
    , m_networkReply( reply )
    , m_pos( 0 )
    , m_timer( new QTimer( this ) )
{
    if ( !m_networkReply->isOpen() ) {
        m_networkReply->open(QIODevice::ReadOnly);
    }

    tLog() << Q_FUNC_INFO;
    Q_ASSERT( m_networkReply->isOpen() );
    Q_ASSERT( m_networkReply->isReadable() );

    if ( m_networkReply->isFinished() )
    {
        // All data is ready, read it!
        m_data = m_networkReply->readAll();
        Q_ASSERT( m_networkReply->atEnd() );
        setStreamSeekable( true );
        setStreamSize( m_data.size() );
        tLog() << Q_FUNC_INFO << "Got data of size:" << m_data.size();
    }
    else
    {
        Q_ASSERT( false );
        // TODO: Connect to finished() signal
    }

    m_timer->setInterval( 0 );
    connect( m_timer, SIGNAL( timeout() ), SLOT( moreData() ) );
}


QNR_IODeviceStream::~QNR_IODeviceStream()
{
    tLog() << Q_FUNC_INFO;
}


void
QNR_IODeviceStream::enoughData()
{
    tLog() << Q_FUNC_INFO;
    m_timer->stop();
}


void
QNR_IODeviceStream::needData()
{
    tLog() << Q_FUNC_INFO;
    m_timer->start();
    moreData();
}

void
QNR_IODeviceStream::reset()
{
    tLog() << Q_FUNC_INFO;
    m_pos = 0;
}


void
QNR_IODeviceStream::seekStream( qint64 offset )
{
    tLog() << Q_FUNC_INFO;
    m_pos = offset;
}

void
QNR_IODeviceStream::moreData()
{
    QByteArray data = m_data.mid( m_pos, BLOCK_SIZE );
    tLog() << Q_FUNC_INFO << data.size();
    m_pos += data.size();
    if (m_data.size() == 0
            // && m_networkReply->atEnd()
            // && m_networkReply->isFinished()
            )
    {
        // We're done.
        endOfData();
        m_timer->stop();
    }
    else
    {
        writeData( data );
    }
}


// vim: sw=4 sts=4 et tw=100
