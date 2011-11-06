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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtGui/QWidget>
#include <QtGui/QIcon>
#include <QtCore/QString>
#include <QtCore/QUuid>

#include "typedefs.h"
#include "dllmacro.h"
#include "tomahawksettings.h"

class SipPlugin;

namespace Tomahawk
{

namespace InfoSystem
{
    class InfoPlugin;
}
    
namespace Accounts
{

enum AccountType { InfoType, SipType };

inline QString generateId( const QString &factoryId )
{
    QString uniq = QUuid::createUuid().toString().mid( 1, 8 );
    return factoryId + "_" + uniq;
}

class DLLEXPORT Account : public QObject
{
    Q_OBJECT
    
public:
    explicit Account( const QString &accountId )
        : QObject()
        , m_enabled( false )
        , m_autoConnect( false )
        , m_accountId( accountId ) {}
    virtual ~Account() {}

    virtual QString accountServiceName() const { QMutexLocker locker( &m_mutex ); return m_accountServiceName; } // e.g. "Twitter", "Last.fm"
    virtual QString accountFriendlyName() const { QMutexLocker locker( &m_mutex ); return m_accountFriendlyName; } // e.g. screen name on the service, JID, etc.
    virtual bool enabled() const { QMutexLocker locker( &m_mutex ); return m_enabled; }
    virtual bool autoConnect() const { QMutexLocker locker( &m_mutex ); return m_autoConnect; }
    virtual QString accountId() const { QMutexLocker locker( &m_mutex ); return m_accountId; }

    virtual QVariantHash configuration() const { QMutexLocker locker( &m_mutex ); return m_configuration; }
    virtual QWidget* configurationWidget() = 0;

    virtual QVariantHash credentials() { QMutexLocker locker( &m_mutex ); return m_credentials; }

    virtual QVariantMap acl() const { QMutexLocker locker( &m_mutex ); return m_acl; }
    virtual QWidget* aclWidget() = 0;

    virtual QIcon icon() const = 0;

    virtual bool canSelfAuthenticate() const = 0;
    virtual bool authenticate() = 0; //if none needed, just return true
    virtual bool isAuthenticated() const = 0;

    virtual Tomahawk::InfoSystem::InfoPlugin* infoPlugin() = 0;
    virtual SipPlugin* sipPlugin() = 0;

    virtual QSet< AccountType > types() const
    {
        QMutexLocker locker( &m_mutex );
        QSet< AccountType > set;
        foreach ( QString type, m_types )
        {
            if ( type == "InfoType" )
                set << InfoType;
            else if ( type == "SipType" )
                set << SipType;
        }
        return set;
    }

    virtual void setAccountServiceName( const QString &serviceName ) { QMutexLocker locker( &m_mutex ); m_accountServiceName = serviceName; }
    virtual void setAccountFriendlyName( const QString &friendlyName )  { QMutexLocker locker( &m_mutex ); m_accountFriendlyName = friendlyName; }
    virtual void setEnabled( bool enabled ) { QMutexLocker locker( &m_mutex ); m_enabled = enabled; }
    virtual void setAutoConnect( bool autoConnect ) { QMutexLocker locker( &m_mutex ); m_autoConnect = autoConnect; }
    virtual void setAccountId( const QString &accountId )  { QMutexLocker locker( &m_mutex ); m_accountId = accountId; }
    virtual void setCredentials( const QVariantHash &credentialHash ) { QMutexLocker locker( &m_mutex ); m_credentials = credentialHash; }

    virtual void setConfiguration( const QVariantHash &configuration ) { QMutexLocker locker( &m_mutex ); m_configuration = configuration; }

    virtual void setAcl( const QVariantMap &acl ) { QMutexLocker locker( &m_mutex ); m_acl = acl; }

    virtual void setTypes( const QSet< AccountType > types )
    {
        QMutexLocker locker( &m_mutex );
        m_types = QStringList();
        foreach ( AccountType type, types )
        {
            switch( type )
            {
                case InfoType:
                    m_types << "InfoType";
                    break;
                case SipType:
                    m_types << "SipType";
                    break;
            }
        }
        syncConfig();
    }

    virtual void sync() { QMutexLocker locker( &m_mutex ); syncConfig(); };

signals:
    void configurationChanged();
    
protected:
    virtual void loadFromConfig( const QString &accountId )
    {
        m_accountId = accountId;
        TomahawkSettings* s = TomahawkSettings::instance();
        s->beginGroup( "accounts/" + m_accountId );
        m_accountFriendlyName = s->value( "accountfriendlyname", QString() ).toString();
        m_enabled = s->value( "enabled", false ).toBool();
        m_autoConnect = s->value( "autoconnect", false ).toBool();
        m_credentials = s->value( "credentials", QVariantHash() ).toHash();
        m_configuration = s->value( "configuration", QVariantHash() ).toHash();
        m_acl = s->value( "acl", QVariantMap() ).toMap();
        m_types = s->value( "types", QStringList() ).toStringList();
        s->endGroup();
    }

    virtual void syncConfig()
    {
        TomahawkSettings* s = TomahawkSettings::instance();
        s->beginGroup( "accounts/" + m_accountId );
        s->setValue( "accountfriendlyname", m_accountFriendlyName );
        s->setValue( "enabled", m_enabled );
        s->setValue( "autoconnect", m_autoConnect );
        s->setValue( "credentials", m_credentials );
        s->setValue( "configuration", m_configuration );
        s->setValue( "acl", m_acl );
        s->setValue( "types", m_types );
        s->endGroup();
        s->sync();
    }

    QString m_accountServiceName;
    QString m_accountFriendlyName;
    bool m_enabled;
    bool m_autoConnect;
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
    // if the user can create multiple
    virtual QIcon icon() const { return QIcon(); }
    virtual bool isUnique() const { return false; }

    virtual Account* createAccount( const QString& accountId = QString() ) = 0;
};

};

};

Q_DECLARE_INTERFACE( Tomahawk::Accounts::AccountFactory, "tomahawk.AccountFactory/1.0" )

#endif