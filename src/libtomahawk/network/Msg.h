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

#include <QByteArray>
#include <QSharedPointer>
#include <QtEndian>
#include <QIODevice>
#include <QVariant>

class Msg;
typedef QSharedPointer<Msg> msg_ptr;

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

    virtual ~Msg()
    {
        //qDebug() << Q_FUNC_INFO;
    }

    /// constructs new msg you wish to send
    static msg_ptr factory( const QByteArray& ba, char f )
    {
        return msg_ptr( new Msg( ba, f ) );
    }

    /// constructs an incomplete new msg that is missing the payload data
    static msg_ptr begin( char* headerToParse )
    {
        quint32 lenBE = *( (quint32*) headerToParse );
        quint8 flags = *( (quint8*) (headerToParse+4) );
        return msg_ptr( new Msg( qFromBigEndian(lenBE), flags ) );
    }

    /// completes msg construction by providing payload data
    void fill( const QByteArray& ba )
    {
        Q_ASSERT( m_incomplete );
        Q_ASSERT( ba.length() == (qint32)m_length );
        m_payload = ba;
        m_incomplete = false;
    }

    /// frames the msg and writes to the wire:
    bool write( QIODevice * device )
    {
        quint32 size  = qToBigEndian( m_length );
        quint8  flags = m_flags;
        if( device->write( (const char*) &size,  sizeof(quint32) ) != sizeof(quint32) ) return false;
        if( device->write( (const char*) &flags, sizeof(quint8) )  != sizeof(quint8)  ) return false;
        if( device->write( (const char*) m_payload.data(), m_length ) != m_length ) return false;
        return true;
    }

    // len(4) + flags(1)
    static quint8 headerSize() { return sizeof(quint32) + sizeof(quint8); }

    quint32 length() const { return m_length; }

    bool is( Flag flag ) { return m_flags & flag; }

    const QByteArray& payload() const
    {
        Q_ASSERT( m_incomplete == false );
        return m_payload;
    }

    QVariant& json();

    char flags() const { return m_flags; }

private:
    /// used when constructing Msg you wish to send
    Msg( const QByteArray& ba, char f )
        :   m_payload( ba ),
            m_length( ba.length() ),
            m_flags( f ),
            m_incomplete( false ),
            m_json_parsed( false)
    {
    }

    /// used when constructung Msg off the wire:
    Msg( quint32 len, quint8 flags )
        :   m_length( len ),
            m_flags( flags ),
            m_incomplete( true ),
            m_json_parsed( false)
    {
    }

    QByteArray m_payload;
    quint32 m_length;
    char m_flags;
    bool m_incomplete;
    QVariant m_json;
    bool m_json_parsed;
};

#endif // MSG_H
