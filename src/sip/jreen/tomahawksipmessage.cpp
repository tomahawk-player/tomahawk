/****************************************************************************
 *
 *  This file is part of qutIM
 *
 *  Copyright (c) 2011 by Nigmatullin Ruslan <euroelessar@gmail.com>
 *
 ***************************************************************************
 *                                                                         *
 *   This file is part of free software; you can redistribute it and/or    *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************
 ****************************************************************************/

#include "tomahawksipmessage.h"

class TomahawkSipMessagePrivate
{
public:
    QString ip;
    int port;
    QString uniqname;
    QString key;
    bool visible;
};

TomahawkSipMessage::TomahawkSipMessage(QString ip, unsigned int port, QString uniqname, QString key, bool visible) : d_ptr(new TomahawkSipMessagePrivate)
{
    Q_D(TomahawkSipMessage);
    d->ip = ip;
    d->port = port;
    d->uniqname = uniqname;
    d->key = key;
    d->visible = visible;
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

QString TomahawkSipMessage::uniqname() const
{
    return d_func()->uniqname;
}

QString TomahawkSipMessage::key() const
{
    return d_func()->key;
}

bool TomahawkSipMessage::visible() const
{
    return d_func()->visible;
}
