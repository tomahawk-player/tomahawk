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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tomahawksipmessage.h"

#include "utils/logger.h"


class TomahawkSipMessagePrivate
{
public:
    QString ip;
    int port;
    QString uniqname;
    QString key;
    bool visible;
};

TomahawkSipMessage::TomahawkSipMessage(const QString &ip, unsigned int port, const QString &uniqname, const QString &key) : d_ptr(new TomahawkSipMessagePrivate)
{
    Q_D(TomahawkSipMessage);
    d->ip = ip;
    d->port = port;
    d->uniqname = uniqname;
    d->key = key;
    d->visible = true;
}

TomahawkSipMessage::TomahawkSipMessage() : d_ptr(new TomahawkSipMessagePrivate)
{
    Q_D(TomahawkSipMessage);
    d->visible = false;
    d->port = -1;
}


TomahawkSipMessage::~TomahawkSipMessage()
{
}

const QString TomahawkSipMessage::ip() const
{
    return d_func()->ip;
}

unsigned int TomahawkSipMessage::port() const
{
    return d_func()->port;
}

const QString TomahawkSipMessage::uniqname() const
{
    return d_func()->uniqname;
}

const QString TomahawkSipMessage::key() const
{
    return d_func()->key;
}

bool TomahawkSipMessage::visible() const
{
    return d_func()->visible;
}
