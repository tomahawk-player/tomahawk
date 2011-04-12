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

#ifndef ZEROCONF_H
#define ZEROCONF_H

#include "sip/SipPlugin.h"
#include "tomahawkzeroconf.h"

#include "../sipdllmacro.h"

#define MYNAME "SIPZEROCONF"

class SIPDLLEXPORT ZeroconfPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )

public:
    ZeroconfPlugin()
        : m_zeroconf( 0 )
        , m_isOnline( false )
        , m_cachedNodes()
    {
        qDebug() << Q_FUNC_INFO;
    }

    virtual ~ZeroconfPlugin()
    {
        qDebug() << Q_FUNC_INFO;
    }
    
    virtual bool isValid() { return true; }
    virtual const QString name();
    virtual const QString friendlyName();
    virtual const QString accountName();

public slots:
    virtual bool connectPlugin( bool startup );
    void disconnectPlugin();
    void checkSettings() {}

    void sendMsg( const QString& to, const QString& msg )
    {
        Q_UNUSED( to );
        Q_UNUSED( msg );
    }

    void broadcastMsg( const QString &msg )
    {
        Q_UNUSED( msg );
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
        Q_UNUSED( jid );
        Q_UNUSED( msg );
    }

private slots:
    void lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid );

private:
    TomahawkZeroconf* m_zeroconf;
    bool m_isOnline;
    QSet< QStringList* > m_cachedNodes;
};

#endif
