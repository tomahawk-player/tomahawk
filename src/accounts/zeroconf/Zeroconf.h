/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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
#include "accounts/Account.h"
#include "TomahawkZeroconf.h"

#include "accounts/AccountDllMacro.h"

#include <QtCore/QTimer>

#define MYNAME "zeroconf"

namespace Tomahawk
{
namespace Accounts
{

class ZeroconfAccount;


class ACCOUNTDLLEXPORT ZeroconfPlugin : public SipPlugin
{
    Q_OBJECT

public:
    ZeroconfPlugin( ZeroconfAccount* acc );

    virtual ~ZeroconfPlugin();

    virtual const QString name() const;
    virtual const QString friendlyName() const;
    virtual const QString accountName() const;
    virtual const QString serviceName() const;
    virtual Account::ConnectionState connectionState() const;
    virtual bool isValid() const { return true; }
#ifndef ENABLE_HEADLESS
    virtual QIcon icon() const;
#endif
    virtual void checkSettings() {}
    virtual void configurationChanged() {}

public slots:
    void connectPlugin();
    void disconnectPlugin();

    void advertise();

    void sendSipInfo( const Tomahawk::peerinfo_ptr&, const SipInfo& ) {}
    void broadcastMsg( const QString & ) {}
    void addContact( const QString &, const QString&  ) {}

private slots:
    void lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid );

private:
    TomahawkZeroconf* m_zeroconf;
    Account::ConnectionState m_state;
    QVector<QStringList> m_cachedNodes;
    QTimer m_advertisementTimer;
};

}
}

#endif
