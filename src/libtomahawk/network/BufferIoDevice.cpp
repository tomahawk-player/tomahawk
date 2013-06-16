/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "BufferIoDevice_p.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QThread>

#include "utils/Logger.h"

// Msgs are framed, this is the size each msg we send containing audio data:
#define BLOCKSIZE 4096


BufferIODevice::BufferIODevice( unsigned int size, QObject* parent )
    : QIODevice( parent )
    , d_ptr( new BufferIODevicePrivate( this, size ) )
{
}


BufferIODevice::~BufferIODevice()
{
    delete d_ptr;
}


bool
BufferIODevice::open( OpenMode mode )
{
    Q_D( BufferIODevice );
    Q_UNUSED( mode );
    QMutexLocker lock( &d->mut );

    qDebug() << Q_FUNC_INFO;
    QIODevice::open( QIODevice::ReadOnly | QIODevice::Unbuffered ); // FIXME?
    return true;
}


void
BufferIODevice::close()
{
    Q_D( BufferIODevice );
    QMutexLocker lock( &d->mut );

    qDebug() << Q_FUNC_INFO;
    QIODevice::close();
}


bool
BufferIODevice::seek( qint64 pos )
{
    Q_D( BufferIODevice );
    qDebug() << Q_FUNC_INFO << pos << d->size;

    if ( pos >= d->size )
        return false;

    int block = blockForPos( pos );
    if ( isBlockEmpty( block ) )
        emit blockRequest( block );

    d->pos = pos;
    qDebug() << "Finished seeking";

    return true;
}


void
BufferIODevice::seeked( int block )
{
    Q_D( BufferIODevice );
    qDebug() << Q_FUNC_INFO << block << d->size;
}


void
BufferIODevice::inputComplete( const QString& errmsg )
{
    Q_D( BufferIODevice );
    qDebug() << Q_FUNC_INFO;
    setErrorString( errmsg );
    d->size = d->received;
    emit readChannelFinished();
}


bool
BufferIODevice::isSequential() const
{
    return false;
}


void
BufferIODevice::addData( int block, const QByteArray& ba )
{
    Q_D( BufferIODevice );
    {
        QMutexLocker lock( &d->mut );

        while ( d->buffer.count() <= block )
            d->buffer << QByteArray();

        d->buffer.replace( block, ba );
    }

    // If this was the last block of the transfer, check if we need to fill up gaps
    if ( block + 1 == maxBlocks() )
    {
        if ( nextEmptyBlock() >= 0 )
        {
            emit blockRequest( nextEmptyBlock() );
        }
    }

    d->received += ba.count();
    emit bytesWritten( ba.count() );
    emit readyRead();
}


qint64
BufferIODevice::bytesAvailable() const
{
    Q_D( const BufferIODevice );

    return d->size - d->pos;
}


qint64
BufferIODevice::readData( char* data, qint64 maxSize )
{
    Q_D( BufferIODevice );
//    qDebug() << Q_FUNC_INFO << m_pos << maxSize << 1;

    if ( atEnd() )
        return 0;

    QByteArray ba;
    ba.append( getData( d->pos, maxSize ) );
    d->pos += ba.count();

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
    Q_D( const BufferIODevice );
    qDebug() << Q_FUNC_INFO << d->size;
    return d->size;
}


bool
BufferIODevice::atEnd() const
{
    Q_D( const BufferIODevice );
//    qDebug() << Q_FUNC_INFO << ( m_size <= m_pos );
    return ( d->size <= d->pos );
}


qint64
BufferIODevice::pos() const
{
    Q_D( const BufferIODevice );
    return d->pos;
}


void
BufferIODevice::clear()
{
    Q_D( BufferIODevice );
    QMutexLocker lock( &d->mut );

    d->pos = 0;
    d->buffer.clear();
}


QIODevice::OpenMode
BufferIODevice::openMode() const
{
    return QIODevice::ReadOnly | QIODevice::Unbuffered;
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
    Q_D( const BufferIODevice );

    int i = 0;
    foreach( const QByteArray& ba, d->buffer )
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
    Q_D( const BufferIODevice );

    int i = d->size / BLOCKSIZE;

    if ( ( d->size % BLOCKSIZE ) > 0 )
        i++;

    return i;
}


bool
BufferIODevice::isBlockEmpty( int block ) const
{
    Q_D( const BufferIODevice );
    if ( block >= d->buffer.count() )
        return true;

    return d->buffer.at( block ).isEmpty();
}


QByteArray
BufferIODevice::getData( qint64 pos, qint64 size )
{
    Q_D( BufferIODevice );
//    qDebug() << Q_FUNC_INFO << pos << size << 1;
    QByteArray ba;
    int block = blockForPos( pos );
    int offset = offsetForPos( pos );

    QMutexLocker lock( &d->mut );
    while( ba.count() < size )
    {
        if ( block > maxBlocks() )
            break;

        if ( isBlockEmpty( block ) )
            break;

        ba.append( d->buffer.at( block++ ).mid( offset ) );
    }

//    qDebug() << Q_FUNC_INFO << pos << size << 2;
    return ba.left( size );
}
