/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QtCore/QObject>

#include "typedefs.h"
#include "dllmacro.h"
#include "infosystem/infosystem.h"
#include "sip/SipPlugin.h"
#include "Account.h"

namespace Tomahawk
{

namespace Accounts
{

class DLLEXPORT AccountManager : public QObject
{
    Q_OBJECT

public:
    static AccountManager* instance();

    explicit AccountManager( QObject *parent );
    virtual ~AccountManager();

    QStringList findPluginFactories();
    void loadPluginFactories( const QStringList &paths );

    void loadFromConfig();
    void initSIP();

    void loadPluginFactory( const QString &path );
    void addAccountPlugin( Account* account );
    Account* loadPlugin( const QString &accountId );
    QString factoryFromId( const QString& accountId ) const;

    QList< Account* > accounts() const { return m_accounts; };
    QList< Account* > accounts( Tomahawk::Accounts::AccountType type ) const { return m_accountsByAccountType[ type ]; }

public slots:
    void connectAll();
    void disconnectAll();
    void toggleAccountsConnected();

signals:
    void added( Tomahawk::Accounts::Account* );
    void removed( Tomahawk::Accounts::Account* );

    void connected( Tomahawk::Accounts::Account* );
    void disconnected( Tomahawk::Accounts::Account* );
    void authError( Tomahawk::Accounts::Account* );

    void stateChanged( Account* p, Accounts::Account::ConnectionState state );

private slots:
    void onStateChanged( Accounts::Account::ConnectionState state );
    void onError( int code, const QString& msg );

    void onSettingsChanged();
private:
    void hookupAccount( Account* ) const;

    QList< Account* > m_accounts;
    QList< Account* > m_enabledAccounts;
    QList< Account* > m_connectedAccounts;
    bool m_connected;

    QHash< AccountType, QList< Account* > > m_accountsByAccountType;
    QHash< QString, AccountFactory* > m_accountFactories;

    static AccountManager* s_instance;
};

};

};

#endif
