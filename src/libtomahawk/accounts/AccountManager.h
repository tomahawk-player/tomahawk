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

#include "Typedefs.h"
#include "DllMacro.h"
#include "infosystem/InfoSystem.h"
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

    void loadFromConfig();
    void initSIP();

    void enableAccount( Account* account );
    void disableAccount( Account* account );

    QList< AccountFactory* > factories() const { return m_accountFactories.values(); }
    bool hasPluginWithFactory( const QString& factory ) const;
    AccountFactory* factoryForAccount( Account* account ) const;

    void addAccount( Account* account );
    void hookupAndEnable( Account* account, bool startup = false ); /// Hook up signals and start the plugin
    void removeAccount( Account* account );

    QList< Account* > accounts() const { return m_accounts; };
    QList< Account* > accounts( Tomahawk::Accounts::AccountType type ) const { return m_accountsByAccountType[ type ]; }

    QList< Account* > accountsFromFactory( Tomahawk::Accounts::AccountFactory* factory ) const;

    /**
     * Returns a new Account for a certain path on disk. This will go through all on-disk resolver account providers
     * to find the most specific account that matches this.
     *
     * The fallback is ResolverAccount, which handles our generic external script resolvers.
     */
    Account* accountFromPath( const QString& path );

    /**
     * Registers an account factory as being able to "handle" resolvers on disk. When accountFromPath is called
     * AccountManager will go through all registered account factories in order until it finds one that can handle the path.
     * This is searched in LIFO order.
     */
    void registerAccountFactoryForFilesystem( AccountFactory* factory );

    void addAccountFactory( AccountFactory* factory );

    Account* zeroconfAccount() const;

    bool isConnected() { return m_connected; }

public slots:
    void connectAll();
    void disconnectAll();
    void toggleAccountsConnected();

signals:
    void ready();

    void added( Tomahawk::Accounts::Account* );
    void removed( Tomahawk::Accounts::Account* );

    void connected( Tomahawk::Accounts::Account* );
    void disconnected( Tomahawk::Accounts::Account* );
    void authError( Tomahawk::Accounts::Account* );

    void stateChanged( Account* p, Accounts::Account::ConnectionState state );

private slots:
    void init();
    void onStateChanged( Tomahawk::Accounts::Account::ConnectionState state );
    void onError( int code, const QString& msg );

    void onSettingsChanged();

private:
    QStringList findPluginFactories();
    void loadPluginFactories( const QStringList &paths );
    void loadPluginFactory( const QString &path );
    QString factoryFromId( const QString& accountId ) const;

    Account* loadPlugin( const QString &accountId );
    void hookupAccount( Account* ) const;

    QList< Account* > m_accounts;
    QList< Account* > m_enabledAccounts;
    QList< Account* > m_connectedAccounts;
    bool m_connected;

    QHash< AccountType, QList< Account* > > m_accountsByAccountType;
    QHash< QString, AccountFactory* > m_accountFactories;
    QList< AccountFactory* > m_factoriesForFilesytem;

    static AccountManager* s_instance;
};

};

};

#endif
