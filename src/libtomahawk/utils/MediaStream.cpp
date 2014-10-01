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

static QString s_aeInfoIdentifier = QString( "MEDIASTREAM" );


MediaStream::MediaStream( QObject* parent )
    : QObject( parent )
    , m_type( Unknown )
    , m_url( QUrl() )
    , m_ioDevice ( 0 )
    , m_started( false )
    , m_bufferingFinished( false )
    , m_eos( false )
    , m_pos( 0 )
    , m_streamSize( 0 )
{
    tDebug() << Q_FUNC_INFO;

}


MediaStream::MediaStream( const QUrl &url )
    : QObject( 0 )
    , m_type(Url)
    , m_ioDevice ( 0 )
    , m_started( false )
    , m_bufferingFinished( false )
    , m_eos( false )
    , m_pos( 0 )
    , m_streamSize( 0 )
{
    tDebug() << Q_FUNC_INFO;

    m_url = url;
}


MediaStream::MediaStream( QIODevice* device )
    : QObject( 0 )
    , m_type(IODevice)
    , m_url( QUrl() )
    , m_ioDevice ( 0 )
    , m_started( false )
    , m_bufferingFinished( false )
    , m_eos( false )
    , m_pos( 0 )
    , m_streamSize( 0 )
{
    tDebug() << Q_FUNC_INFO;

    m_ioDevice = device;
    QObject::connect( m_ioDevice, SIGNAL( readChannelFinished() ), this, SLOT( bufferingFinished() ) );
}


MediaStream::MediaStream( const MediaStream& copy )
    : QObject( copy.parent() )
{
    m_type = copy.m_type;
    m_url = copy.m_url;
    m_ioDevice = copy.m_ioDevice;
    m_started = copy.m_started;
    m_bufferingFinished = copy.m_bufferingFinished;
    m_eos = copy.m_eos;
    m_pos = copy.m_pos;
    m_streamSize = copy.m_streamSize;

    if ( m_type == IODevice ) {
        QObject::connect( m_ioDevice, SIGNAL( readChannelFinished() ), this, SLOT( bufferingFinished() ) );
    }
}


MediaStream::~MediaStream()
{
    tDebug() << Q_FUNC_INFO;
}


MediaStream::MediaType
MediaStream::type()
{
    tDebug() << Q_FUNC_INFO;
    return m_type;
}


QUrl
MediaStream::url()
{
    tDebug() << Q_FUNC_INFO;
    return m_url;
}


qint64
MediaStream::streamSize()
{
    tDebug() << Q_FUNC_INFO;

    return m_streamSize;
}


void
MediaStream::setStreamSize( qint64 size )
{
    tDebug() << Q_FUNC_INFO;

    m_streamSize = size;
}


void
MediaStream::endOfData()
{
    tDebug() << Q_FUNC_INFO;

    m_eos = true;
}


void MediaStream::bufferingFinished()
{
    tDebug() << Q_FUNC_INFO;

    m_bufferingFinished = true;
}


int
MediaStream::readCallback ( void* data, const char* cookie, int64_t* dts, int64_t* pts, unsigned* flags, size_t* bufferSize, void** buffer )
{
    Q_UNUSED(cookie);
    Q_UNUSED(dts);
    Q_UNUSED(pts);
    Q_UNUSED(flags);

    MediaStream* that = static_cast < MediaStream * > ( data );
    qint64 bufsize = 0;
    *bufferSize = 0;

//    tDebug() << Q_FUNC_INFO << " ---- type : " << that->m_type;

    if ( that->m_eos == true ) {
        return -1;
    }

    if ( that->m_type == Stream ) {
        bufsize = that->needData(buffer);
    }
    else if ( that->m_type == IODevice ) {
        /*
        *buffer = new char[BLOCK_SIZE];
        bufsize = that->m_ioDevice->read( (char*)*buffer, BLOCK_SIZE );
        */
        bufsize = that->m_ioDevice->read( that->m_buffer, BLOCK_SIZE );
        *buffer = that->m_buffer;
//        tDebug() << "readCallback(QIODevice) returning bufsize : " << bufsize;
    }

    if ( bufsize > 0 ) {
        that->m_started = true;
    }
    if ( bufsize == 0 && that->m_started && that->m_bufferingFinished == true ) {
        that->m_eos = true;
        return -1;
    }
    if ( bufsize < 0 ) {
        that->m_eos = true;
        return -1;
    }

    *bufferSize = bufsize;
    return 0;
}


int
MediaStream::readDoneCallback ( void *data, const char *cookie, size_t bufferSize, void *buffer )
{
//    tDebug() << Q_FUNC_INFO;

    Q_UNUSED(cookie);
    Q_UNUSED(bufferSize);

    MediaStream* that = static_cast < MediaStream * > ( data );

    if ( ( that->m_type == Stream/* || that->m_type == IODevice*/ ) && buffer != 0 && bufferSize > 0 ) {
//      TODO : causes segfault
//        tDebug() << "buffer : " << buffer;
        delete static_cast<char *>(buffer);
    }

    return 0;
}


int
MediaStream::seekCallback ( void *data, const uint64_t pos )
{
    tDebug() << Q_FUNC_INFO;

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
