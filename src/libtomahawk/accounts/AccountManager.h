/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include <QFlags>
#include <QObject>

#include "Typedefs.h"
#include "DllMacro.h"
#include "infosystem/InfoSystem.h"
#include "Account.h"

namespace Tomahawk
{

namespace Accounts
{

class CredentialsManager;

class DLLEXPORT AccountManager : public QObject
{
    Q_OBJECT

public:
    enum DisconnectReason {
        Disconnected,
        Disabled
    };

    static AccountManager* instance();

    explicit AccountManager( QObject *parent );
    virtual ~AccountManager();

    void loadFromConfig();
    void initSIP(); //only call this after isReadyForSip returns true

    void enableAccount( Account* account );
    void disableAccount( Account* account );

    QList< AccountFactory* > factories() const { return m_accountFactories.values(); }
    bool hasPluginWithFactory( const QString& factory ) const;
    AccountFactory* factoryForAccount( Account* account ) const;

    void addAccount( Account* account );
    void hookupAndEnable( Account* account, bool startup = false ); /// Hook up signals and start the plugin
    void removeAccount( Account* account );

    QList< Account* > accounts() const { return m_accounts; }
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

    bool isConnected() const { return m_connected; }        //for use by TomahawkApp during startup
    bool isReadyForSip() const { return m_readyForSip; }    //for use by TomahawkApp during startup
    bool isReady() const { return m_completelyReady; }

    CredentialsManager* credentialsManager() const { return m_creds; }
    ConfigStorage* configStorageForAccount( const QString& accountId );

public slots:
    void connectAll();
    void disconnectAll();
    void toggleAccountsConnected();

signals:
    void readyForFactories(); //this happens first, right before loading accounts from config
    void readyForSip();       //then this, so TomahawkApp can call initSIP if Servent is ready
    void ready();             //finally, when everything is done

    void added( Tomahawk::Accounts::Account* );
    void removed( Tomahawk::Accounts::Account* );

    void connected( Tomahawk::Accounts::Account* );
    void disconnected( Tomahawk::Accounts::Account*, Tomahawk::Accounts::AccountManager::DisconnectReason );
    void authError( Tomahawk::Accounts::Account* );

    void stateChanged( Account* p, Accounts::Account::ConnectionState state );

private slots:
    void init();
    void onStateChanged( Tomahawk::Accounts::Account::ConnectionState state );
    void onError( int code, const QString& msg );
    void finishLoadingFromConfig( const QString& cs );

    void onSettingsChanged();

private:
    void loadPluginFactories( const QStringList &paths );
    void loadPluginFactory( const QString &path );
    QString factoryFromId( const QString& accountId ) const;


    Account* loadPlugin( const QString& accountId );
    void hookupAccount( Account* ) const;

    CredentialsManager* m_creds;

    QList< Account* > m_accounts;
    QList< Account* > m_enabledAccounts;
    QList< Account* > m_connectedAccounts;
    bool m_connected;
    bool m_readyForSip;
    bool m_completelyReady;

    QHash< AccountType, QList< Account* > > m_accountsByAccountType;
    QHash< QString, AccountFactory* > m_accountFactories;
    QList< AccountFactory* > m_factoriesForFilesytem;

    QHash< QString, ConfigStorage* > m_configStorageById;
    QSet< QString > m_configStorageLoading;

    static AccountManager* s_instance;
};

};

};

#endif
