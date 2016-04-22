/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "MediaStream.h"

#include "utils/Logger.h"

#define BLOCK_SIZE 1048576

MediaStream::MediaStream( QObject* parent )
    : QObject( parent )
    , m_type( Unknown )
    , m_ioDevice ( nullptr )
{
}


MediaStream::MediaStream( const QUrl &url )
    : QObject( nullptr )
    , m_type( Url )
    , m_url( url )
    , m_ioDevice ( nullptr )
{
}


MediaStream::MediaStream( QIODevice* device, bool bufferingFinished )
    : QObject( nullptr )
    , m_type( IODevice )
    , m_ioDevice ( device )
    , m_bufferingFinished( bufferingFinished )
{
    if ( !bufferingFinished )
    {
        QObject::connect( m_ioDevice, SIGNAL( readChannelFinished() ), this, SLOT( bufferingFinished() ) );
    }
}


MediaStream::~MediaStream()
{
}


MediaStream::MediaType
MediaStream::type() const
{
    return m_type;
}


QUrl
MediaStream::url() const
{
    return m_url;
}


qint64
MediaStream::streamSize() const
{
    return m_streamSize;
}


void
MediaStream::setStreamSize( qint64 size )
{
    m_streamSize = size;
}


void
MediaStream::endOfData()
{
    m_eos = true;
}


void
MediaStream::bufferingFinished()
{
    m_bufferingFinished = true;
}


int
MediaStream::readCallback ( const char* cookie, int64_t* dts, int64_t* pts, unsigned* flags, size_t* bufferSize, void** buffer )
{
    Q_UNUSED(cookie);
    Q_UNUSED(dts);
    Q_UNUSED(pts);
    Q_UNUSED(flags);

    qint64 bufsize = 0;
    *bufferSize = 0;

    if ( m_eos == true )
    {
        return -1;
    }

    if ( m_type == Stream )
    {
        bufsize = needData( buffer );
    }
    else if ( m_type == IODevice )
    {
        bufsize = m_ioDevice->read( m_buffer, BLOCK_SIZE );
        *buffer = m_buffer;
    }

    if ( bufsize > 0 )
    {
        m_started = true;
    }
    if ( m_type == IODevice && bufsize == 0 && m_started && m_bufferingFinished == true )
    {
        m_eos = true;
        return -1;
    }
    if ( bufsize < 0 )
    {
        m_eos = true;
        return -1;
    }

    *bufferSize = bufsize;
    return 0;
}


int
MediaStream::readDoneCallback ( const char *cookie, size_t bufferSize, void *buffer )
{
    Q_UNUSED(cookie);

    if ( ( m_type == Stream ) && buffer != nullptr && bufferSize > 0 ) {
        delete[] reinterpret_cast< char* >( buffer );
    }

    return 0;
}


int
MediaStream::seekCallback ( void *data, const uint64_t pos )
{
    MediaStream* that = static_cast < MediaStream * > ( data );

    if ( that->m_type == Stream && static_cast < int64_t > ( pos ) > that->streamSize() ) {
        return -1;
    }

    that->m_started = false;
    that->m_pos = pos;
    if ( that->m_type == IODevice ) {
        that->m_ioDevice->seek(pos);
    }

    return 0;
}
