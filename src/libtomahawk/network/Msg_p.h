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

#ifndef MSG_P_H
#define MSG_P_H

#include "Msg.h"

class MsgPrivate
{
    friend class MsgProcessor;

public:
    MsgPrivate( Msg* q, const QByteArray& ba, char f )
        : q_ptr ( q )
        , payload( ba )
        , length( ba.length() )
        , flags( f )
        , incomplete( false )
        , json_parsed( false )
    {
    }

    MsgPrivate( Msg* q, quint32 len, quint8 flags )
        : q_ptr( q)
        , length( len )
        , flags( flags )
        , incomplete( true )
        , json_parsed( false)
    {
    }

    Msg* q_ptr;
    Q_DECLARE_PUBLIC ( Msg )

private:
    QByteArray payload;
    quint32 length;
    char flags;
    bool incomplete;
    QVariant json;
    bool json_parsed;
};

#endif // MSG_P_H
