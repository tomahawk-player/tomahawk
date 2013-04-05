/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "Typedefs.h"
#include "DllMacro.h"

#include <QObject>
#include <QVariantMap>
#include <QWidget>
#include <QIcon>
#include <QString>
#include <QUuid>
#include <QMutex>


class SipPlugin;
class AccountConfigWidget;

namespace Tomahawk
{

namespace Accounts
{

enum AccountType
{
    NoType = 0x00,

    InfoType = 0x01,
    SipType = 0x02,
    ResolverType = 0x04,
    StatusPushType = 0x08
};

DLLEXPORT QString accountTypeToString( AccountType type );

Q_DECLARE_FLAGS(AccountTypes, AccountType);

inline QString generateId( const QString& factoryId )
{
    QString uniq = QUuid::createUuid().toString().mid( 1, 8 );
    return factoryId + "_" + uniq;
}

class DLLEXPORT Account : public QObject
{
    Q_OBJECT

public:
    enum AuthErrorCode { AuthError, ConnectionError };
    enum ConnectionState { Disconnected, Connecting, Connected, Disconnecting };

    explicit Account( const QString &accountId );
    virtual ~Account();

    QString accountServiceName() const { QMutexLocker locker( &m_mutex ); return m_accountServiceName; } // e.g. "Twitter", "Last.fm"
    QString accountFriendlyName() const { QMutexLocker locker( &m_mutex ); return m_accountFriendlyName; } // e.g. screen name on the service, JID, etc.
    bool enabled() const { QMutexLocker locker( &m_mutex ); return m_enabled; }
    QString accountId() const { QMutexLocker locker( &m_mutex ); return m_accountId; }

    QVariantHash configuration() const { QMutexLocker locker( &m_mutex ); return m_configuration; }

    /**
     * Configuration widgets can have a "dataError( bool )" signal to enable/disable the OK button in their wrapper dialogs.
     */
#ifndef ENABLE_HEADLESS
    virtual AccountConfigWidget* configurationWidget() = 0;
    virtual QWidget* aboutWidget() { return 0; }
    virtual QWidget* aclWidget() = 0;
    virtual QPixmap icon() const = 0;
#endif
    virtual QString description() const { return QString(); }
    virtual QString author() const { return QString(); }
    virtual QString version() const { return QString(); }

    virtual void saveConfig() {} // called when the widget has been edited. save values from config widget, call sync() to write to disk account generic settings

    QVariantHash credentials() const { QMutexLocker locker( &m_mutex ); return m_credentials; }

    QVariantMap acl() const { QMutexLocker locker( &m_mutex ); return m_acl; }

    virtual ConnectionState connectionState() const = 0;
    virtual bool isAuthenticated() const = 0;

    virtual QString errorMessage() const { QMutexLocker locker( &m_mutex ); return m_cachedError; }

    virtual Tomahawk::InfoSystem::InfoPluginPtr infoPlugin() = 0;
    virtual SipPlugin* sipPlugin() = 0;

    // Some accounts cannot be enabled if authentication fails. Return true after failing to authenticate
    // if this is the case, and the account will not be enabled
    virtual bool preventEnabling() const { return false; }

    AccountTypes types() const;

    void setAccountServiceName( const QString &serviceName ) { QMutexLocker locker( &m_mutex ); m_accountServiceName = serviceName; }
    void setAccountFriendlyName( const QString &friendlyName )  { QMutexLocker locker( &m_mutex ); m_accountFriendlyName = friendlyName; }
    void setEnabled( bool enabled ) { QMutexLocker locker( &m_mutex ); m_enabled = enabled; }
    void setAccountId( const QString &accountId )  { QMutexLocker locker( &m_mutex ); m_accountId = accountId; }
    void setCredentials( const QVariantHash &credentialHash ) { QMutexLocker locker( &m_mutex ); m_credentials = credentialHash; }
    void setConfiguration( const QVariantHash &configuration ) { QMutexLocker locker( &m_mutex ); m_configuration = configuration; }
    void setAcl( const QVariantMap &acl ) { QMutexLocker locker( &m_mutex ); m_acl = acl; }
    void setTypes( AccountTypes types );

    void sync() { QMutexLocker locker( &m_mutex ); syncConfig(); };

    /**
     * Removes all the settings held in the config file for this account instance
     *
     * Re-implement if you have saved additional files or config settings outside the built-in ones
     */
    virtual void removeFromConfig();

public slots:
    virtual void authenticate() = 0;
    virtual void deauthenticate() = 0;

signals:
    void error( int errorId, const QString& errorStr );
    void connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState state );

    void configurationChanged();

protected:
    virtual void loadFromConfig( const QString &accountId );
    virtual void syncConfig();

private slots:
    void onConnectionStateChanged( Tomahawk::Accounts::Account::ConnectionState );
    void onError( int, const QString& );

private:
    QString m_accountServiceName;
    QString m_accountFriendlyName;
    QString m_cachedError;
    bool m_enabled;
    QString m_accountId;
    QVariantHash m_credentials;
    QVariantHash m_configuration;
    QVariantMap m_acl;
    QStringList m_types;
    mutable QMutex m_mutex;
};

class DLLEXPORT AccountFactory : public QObject
{
    Q_OBJECT
public:
    AccountFactory() {}
    virtual ~AccountFactory() {}

    // display name for plugin
    virtual QString prettyName() const = 0;
    // internal name
    virtual QString factoryId() const = 0;
    // description to be shown when user views a list of account types
    virtual QString description() const = 0;
    // if the user can create multiple
    virtual bool isUnique() const { return false; }

    virtual QPixmap icon() const { return QPixmap(); }
    virtual bool allowUserCreation() const { return true; }

    // What are the supported types for accounts this factory creates?
    virtual AccountTypes types() const = 0;

    virtual Account* createAccount( const QString& accountId = QString() ) = 0;

    /// If this resolver type accepts this path on disk (For general and special resolver accounts)
    virtual bool acceptsPath( const QString& ) const { return false; }
    virtual Account* createFromPath( const QString& ) { return 0; }
};

};

};

Q_DECLARE_INTERFACE( Tomahawk::Accounts::AccountFactory, "tomahawk.AccountFactory/1.0" )

Q_DECLARE_METATYPE( Tomahawk::Accounts::Account* )
Q_DECLARE_METATYPE( QList< Tomahawk::Accounts::Account* > )
Q_DECLARE_METATYPE( Tomahawk::Accounts::AccountTypes )

#endif
