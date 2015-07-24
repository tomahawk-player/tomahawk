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

// Feed VLC in 1MiB blocks
#define BLOCK_SIZE 1048576

QNR_IODeviceStream::QNR_IODeviceStream( const QSharedPointer<QNetworkReply>& reply, QObject* parent )
    : MediaStream( parent )
    , m_networkReply( reply )
{
    m_type = MediaStream::Stream;

    if ( !m_networkReply->isOpen() )
    {
        m_networkReply->open( QIODevice::ReadOnly );
    }

    Q_ASSERT( m_networkReply->isOpen() );
    Q_ASSERT( m_networkReply->isReadable() );

    if ( m_networkReply->isFinished() )
    {
        // All data is ready, read it!
        m_data = m_networkReply->readAll();
        Q_ASSERT( m_networkReply->atEnd() );
//TODO        setStreamSeekable( true );
        setStreamSize( m_data.size() );
    }
    else
    {
        // TODO: Connect to finished() signal
        QVariant contentLength = m_networkReply->header( QNetworkRequest::ContentLengthHeader );
        if ( contentLength.isValid() && contentLength.toLongLong() > 0 )
        {
//TODO            setStreamSize( contentLength.toLongLong() );
        }
        // Just consume all data that is already available.
        m_data = m_networkReply->readAll();
        connect( m_networkReply.data(), SIGNAL( readyRead() ), SLOT( readyRead() ) );
    }
}


QNR_IODeviceStream::~QNR_IODeviceStream()
{
}


void
QNR_IODeviceStream::seekStream( qint64 offset )
{
    QMutexLocker locker( &m_mutex );
    m_pos = offset;
}


qint64
QNR_IODeviceStream::needData( void** buffer )
{
    QMutexLocker locker( &m_mutex );
    QByteArray data = m_data.mid( m_pos, BLOCK_SIZE );
    m_pos += data.size();
    if ( ( data.size() == 0 ) && m_networkReply->atEnd() && m_networkReply->isFinished() )
    {
        // We're done.
        endOfData();
        return 0;
    }

    if ( data.size() > 0 ) 
    {
        *buffer = new char[data.size()];
        memcpy(*buffer, data.data(), data.size());
    }
    return data.size();
}


void
QNR_IODeviceStream::readyRead()
{
    QMutexLocker locker( &m_mutex );
    m_data += m_networkReply->readAll();
}
