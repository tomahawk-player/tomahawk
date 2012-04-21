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

#include "BufferIoDevice.h"

#include <QCoreApplication>
#include <QThread>

#include "utils/Logger.h"

// Msgs are framed, this is the size each msg we send containing audio data:
#define BLOCKSIZE 4096


BufferIODevice::BufferIODevice( unsigned int size, QObject* parent )
    : QIODevice( parent )
    , m_size( size )
    , m_received( 0 )
    , m_pos( 0 )
{
}


bool
BufferIODevice::open( OpenMode mode )
{
    Q_UNUSED( mode );
    QMutexLocker lock( &m_mut );

    qDebug() << Q_FUNC_INFO;
    QIODevice::open( QIODevice::ReadOnly | QIODevice::Unbuffered ); // FIXME?
    return true;
}


void
BufferIODevice::close()
{
    QMutexLocker lock( &m_mut );

    qDebug() << Q_FUNC_INFO;
    QIODevice::close();
}


bool
BufferIODevice::seek( qint64 pos )
{
    qDebug() << Q_FUNC_INFO << pos << m_size;

    if ( pos >= m_size )
        return false;

    int block = blockForPos( pos );
    if ( isBlockEmpty( block ) )
        emit blockRequest( block );

    m_pos = pos;
    qDebug() << "Finished seeking";

    return true;
}


void
BufferIODevice::seeked( int block )
{
    qDebug() << Q_FUNC_INFO << block << m_size;
}


void
BufferIODevice::inputComplete( const QString& errmsg )
{
    qDebug() << Q_FUNC_INFO;
    setErrorString( errmsg );
    m_size = m_received;
    emit readChannelFinished();
}


void
BufferIODevice::addData( int block, const QByteArray& ba )
{
    {
        QMutexLocker lock( &m_mut );

        while ( m_buffer.count() <= block )
            m_buffer << QByteArray();

        m_buffer.replace( block, ba );
    }

    // If this was the last block of the transfer, check if we need to fill up gaps
    if ( block + 1 == maxBlocks() )
    {
        if ( nextEmptyBlock() >= 0 )
        {
            emit blockRequest( nextEmptyBlock() );
        }
    }

    m_received += ba.count();
    emit bytesWritten( ba.count() );
    emit readyRead();
}


qint64
BufferIODevice::bytesAvailable() const
{
    return m_size - m_pos;
}


qint64
BufferIODevice::readData( char* data, qint64 maxSize )
{
//    qDebug() << Q_FUNC_INFO << m_pos << maxSize << 1;

    if ( atEnd() )
        return 0;

    QByteArray ba;
    ba.append( getData( m_pos, maxSize ) );
    m_pos += ba.count();

//    qDebug() << Q_FUNC_INFO << maxSize << ba.count() << 2;
    memcpy( data, ba.data(), ba.count() );

    return ba.count();
}


qint64
BufferIODevice::writeData( const char* data, qint64 maxSize )
{
    Q_UNUSED( data );
    Q_UNUSED( maxSize );
    // call addData instead
    Q_ASSERT( false );
    return 0;
}


qint64
BufferIODevice::size() const
{
    qDebug() << Q_FUNC_INFO << m_size;
    return m_size;
}


bool
BufferIODevice::atEnd() const
{
//    qDebug() << Q_FUNC_INFO << ( m_size <= m_pos );
    return ( m_size <= m_pos );
}


void
BufferIODevice::clear()
{
    QMutexLocker lock( &m_mut );

    m_pos = 0;
    m_buffer.clear();
}


unsigned int
BufferIODevice::blockSize()
{
    return BLOCKSIZE;
}


int
BufferIODevice::blockForPos( qint64 pos ) const
{
    // 0 / 4096 -> block 0
    // 4095 / 4096 -> block 0
    // 4096 / 4096 -> block 1

    return pos / BLOCKSIZE;
}


int
BufferIODevice::offsetForPos( qint64 pos ) const
{
    // 0 % 4096 -> offset 0
    // 4095 % 4096 -> offset 4095
    // 4096 % 4096 -> offset 0

    return pos % BLOCKSIZE;
}


int
BufferIODevice::nextEmptyBlock() const
{
    int i = 0;
    foreach( const QByteArray& ba, m_buffer )
    {
        if ( ba.isEmpty() )
            return i;

        i++;
    }

    if ( i == maxBlocks() )
        return -1;

    return i;
}


int
BufferIODevice::maxBlocks() const
{
    int i = m_size / BLOCKSIZE;

    if ( ( m_size % BLOCKSIZE ) > 0 )
        i++;

    return i;
}


bool
BufferIODevice::isBlockEmpty( int block ) const
{
    if ( block >= m_buffer.count() )
        return true;

    return m_buffer.at( block ).isEmpty();
}


QByteArray
BufferIODevice::getData( qint64 pos, qint64 size )
{
//    qDebug() << Q_FUNC_INFO << pos << size << 1;
    QByteArray ba;
    int block = blockForPos( pos );
    int offset = offsetForPos( pos );

    QMutexLocker lock( &m_mut );
    while( ba.count() < size )
    {
        if ( block > maxBlocks() )
            break;

        if ( isBlockEmpty( block ) )
            break;

        ba.append( m_buffer.at( block++ ).mid( offset ) );
    }

//    qDebug() << Q_FUNC_INFO << pos << size << 2;
    return ba.left( size );
}
