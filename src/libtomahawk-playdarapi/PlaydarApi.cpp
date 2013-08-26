/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#include "PlaydarApi_p.h"

#include "utils/Logger.h"

PlaydarApi::PlaydarApi( QHostAddress ha, qint16 port, QObject* parent )
    : QObject( parent )
    , d_ptr( new PlaydarApiPrivate( this ) )
{
    Q_D( PlaydarApi );

    d->ha = ha;
    d->port = port;
}


PlaydarApi::~PlaydarApi()
{
}


void
PlaydarApi::start()
{
    Q_D( PlaydarApi );
    if ( !d->session.isNull() )
    {
        tLog() << "HTTPd session already exists, returning";
        return;
    }

    d->session.reset( new QxtHttpSessionManager() );
    d->connector.reset( new QxtHttpServerConnector() );
    if ( d->session.isNull() || d->connector.isNull() )
    {
        if ( !d->session.isNull() )
            delete d->session.data();
        if ( !d->connector.isNull() )
            delete d->connector.data();
        tLog() << "Failed to start HTTPd, could not create object";
        return;
    }

    d->session->setListenInterface( d->ha );
    d->session->setPort( d->port );
    d->session->setConnector( d->connector.data() );

    d->instance.reset( new Api_v1( d->session.data() ) );
    d->session->setStaticContentService( d->instance.data() );

    tLog() << "Starting HTTPd on" << d->session->listenInterface().toString() << d->session->port();
    d->session->start();
}
