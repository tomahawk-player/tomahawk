/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "Msg_p.h"

#include <qjson/parser.h>

#include <QtEndian>

Msg::Msg( const QByteArray& ba, char f )
    : d_ptr( new MsgPrivate( this, ba, f ) )
{
}

Msg::Msg( quint32 len, quint8 flags )
    : d_ptr( new MsgPrivate( this, len, flags ) )
{
}


Msg::~Msg()
{
    delete d_ptr;
}


msg_ptr
Msg::factory( const QByteArray& ba, char f )
{
    return msg_ptr( new Msg( ba, f ) );
}


msg_ptr
Msg::begin( char* headerToParse )
{
    quint32 lenBE = *( (quint32*) headerToParse );
    quint8 flags = *( (quint8*) (headerToParse+4) );
    return msg_ptr( new Msg( qFromBigEndian(lenBE), flags ) );
}


void
Msg::fill( const QByteArray& ba )
{
    Q_D( Msg );
    Q_ASSERT( d->incomplete );
    Q_ASSERT( ba.length() == (qint32)d->length );
    d->payload = ba;
    d->incomplete = false;
}


bool
Msg::write( QIODevice * device )
{
    Q_D( Msg );
    quint32 size  = qToBigEndian( d->length );
    quint8  flags = d->flags;
    if( device->write( (const char*) &size,  sizeof(quint32) ) != sizeof(quint32) ) return false;
    if( device->write( (const char*) &flags, sizeof(quint8) )  != sizeof(quint8)  ) return false;
    if( device->write( (const char*) d->payload.data(), d->length ) != d->length ) return false;
    return true;
}


quint8
Msg::headerSize()
{
    return sizeof(quint32) + sizeof(quint8);
}


quint32
Msg::length() const
{
    Q_D( const Msg );

    return d->length;
}


bool
Msg::is( Flag flag )
{
    Q_D( Msg );

    return d->flags & flag;
}


const QByteArray&
Msg::payload() const
{
    Q_D( const Msg );
    Q_ASSERT( d->incomplete == false );
    return d->payload;
}


QVariant&
Msg::json()
{
    Q_D( Msg );
    Q_ASSERT( is(JSON) );
    Q_ASSERT( !is(COMPRESSED) );

    if( !d->json_parsed )
    {
        QJson::Parser p;
        bool ok;
        d->json = p.parse( d->payload, &ok );
        d->json_parsed = true;
    }
    return d->json;
}


char
Msg::flags() const
{
    Q_D( const Msg );
    return d->flags;
}
