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

/*
    Msg is a wire msg used by p2p connections.
    Msgs have a 5-byte header:
    - 4 bytes length, big endian
    - 1 byte flags

    Flags indicate if the payload is compressed/json/etc.

    Use static factory method to create, pass around shared pointers: msp_ptr
*/

#ifndef MSG_H
#define MSG_H

#include "Typedefs.h"

#include <QSharedPointer>

class MsgPrivate;
class QByteArray;
class QIODevice;

class Msg
{
    friend class MsgProcessor;

public:
    enum Flag
    {
        RAW = 1,
        JSON = 2,
        FRAGMENT = 4,
        COMPRESSED = 8,
        DBOP = 16,
        PING = 32,
        RESERVED_1 = 64,
        SETUP = 128 // used to handshake/auth the connection prior to handing over to Connection subclass
    };

    virtual ~Msg();

    /**
     * constructs new msg you wish to send
     */
    static msg_ptr factory( const QByteArray& ba, char f );

    /**
     * constructs an incomplete new msg that is missing the payload data
     */
    static msg_ptr begin( char* headerToParse );

    /**
     * completes msg construction by providing payload data
     */
    void fill( const QByteArray& ba );

    /**
     * frames the msg and writes to the wire:
     */
    bool write( QIODevice * device );

    // len(4) + flags(1)
    static quint8 headerSize();

    quint32 length() const;

    bool is( Flag flag );

    const QByteArray& payload() const;

    QVariant& json();

    char flags() const;

private:
    /**
     * Used when constructing Msg you wish to send
     */
    Msg( const QByteArray& ba, char f );

    /**
     * used when constructung Msg off the wire:
     */
    Msg( quint32 len, quint8 flags );

    Q_DECLARE_PRIVATE( Msg )
    MsgPrivate* d_ptr;
};

#endif // MSG_H
