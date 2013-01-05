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
    MsgProcessor is a FIFO queue of msg_ptr, you .add() a msg_ptr, and
    it emits done(msg_ptr) for each msg, preserving the order.

    It can be configured to auto-compress, or de-compress msgs for sending
    or receiving.

    It uses QtConcurrent, but preserves msg order.

    NOT threadsafe.
*/
#ifndef MSGPROCESSOR_H
#define MSGPROCESSOR_H

#include "Msg.h"

#include <QObject>

class MsgProcessor : public QObject
{
Q_OBJECT
public:
    enum Mode
    {
        NOTHING = 0,
        COMPRESS_IF_LARGE = 1,
        UNCOMPRESS_ALL = 2,
        PARSE_JSON = 4
    };

    explicit MsgProcessor( quint32 mode = NOTHING, quint32 t = 512 );

    void setMode( quint32 m ) { m_mode = m ; }

    static msg_ptr process( msg_ptr msg, quint32 mode, quint32 threshold );

    int length() const { return m_msgs.length(); }

signals:
    void ready( msg_ptr );
    void empty();

public slots:
    void append( msg_ptr msg );
    void processed();

private:
    void handleProcessedMsg( msg_ptr msg );

    quint32 m_mode;
    quint32 m_threshold;
    QList<msg_ptr> m_msgs;
    QMap< Msg*, bool> m_msg_ready;
    unsigned int m_totmsgsize;
};

#endif // MSGPROCESSOR_H
