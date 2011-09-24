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
#include <QtCore/QString>
#include <QtCore/QUuid>

#include "typedefs.h"
#include "dllmacro.h"
#include "infosystem/infosystem.h"
#include "sip/SipPlugin.h"
#include "tomahawksettings.h"

namespace Tomahawk
{

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
        , m_autoConnect( false )
        , m_accountId( accountId ) {}
    virtual ~Account() {}

    virtual QString accountServiceName() const { return m_accountServiceName; } // e.g. "Twitter", "Last.fm"
    virtual QString accountFriendlyName() const { return m_accountFriendlyName; } // e.g. screen name on the service, JID, etc.
    virtual bool autoConnect() const { return m_autoConnect; }
    virtual QString accountId() const { return m_accountId; }

    virtual QVariantHash configuration() const { return m_configuration; }
    virtual QWidget* configurationWidget() = 0;

    virtual QVariantHash credentials() { return m_credentials; }

    virtual QVariantMap acl() const { return m_acl; }
    virtual QWidget* aclWidget() = 0;

    virtual QIcon icon() const = 0;

    virtual bool authenticate() = 0; //if none needed, just return true
    virtual bool isAuthenticated() = 0;

    virtual Tomahawk::InfoSystem::InfoPlugin* infoPlugin() = 0;
    virtual SipPlugin* sipPlugin() = 0;

    virtual QSet< AccountType > types() const
    {
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

signals:
    void configurationChanged();

protected:
    virtual void setAccountServiceName( const QString &serviceName ) { m_accountServiceName = serviceName; }
    virtual void setAccountFriendlyName( const QString &friendlyName )  { m_accountFriendlyName = friendlyName; }
    virtual void setAutoConnect( bool autoConnect ) { m_autoConnect = autoConnect; }
    virtual void setAccountId( const QString &accountId )  { m_accountId = accountId; }
    virtual void setCredentials( const QVariantHash &credentialHash ) { m_credentials = credentialHash; }

    virtual void setConfiguration( const QVariantHash &configuration ) { m_configuration = configuration; }

    virtual void setAcl( const QVariantMap &acl ) { m_acl = acl; }
    
    virtual void setTypes( const QSet< AccountType > types )
    {
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

    virtual void loadFromConfig( const QString &accountId )
    {
        m_accountId = accountId;
        TomahawkSettings* s = TomahawkSettings::instance();
        s->beginGroup( "accounts/" + m_accountId );
        m_accountFriendlyName = s->value( "accountFriendlyName", QString() ).toString();
        m_autoConnect = s->value( "autoConnect", false ).toBool();
        m_credentials = s->value( "credentials", QVariantHash() ).toHash();
        m_configuration = s->value( "configuration", QVariantHash() ).toHash();
        m_acl = s->value( "acl", QVariantMap() ).toMap();
        m_types = s->value( "types", QStringList() ).toStringList();
        s->endGroup();
        s->sync();
    }
    
    virtual void syncConfig()
    {
        TomahawkSettings* s = TomahawkSettings::instance();
        s->beginGroup( "accounts/" + m_accountId );
        s->setValue( "accountFriendlyName", m_accountFriendlyName );
        s->setValue( "autoConnect", m_autoConnect );
        s->setValue( "credentials", m_credentials );
        s->setValue( "configuration", m_configuration );
        s->setValue( "acl", m_acl );
        s->setValue( "types", m_types );
        s->endGroup();
        s->sync();

        emit configurationChanged();
    }
    
    QString m_accountServiceName;
    QString m_accountFriendlyName;
    bool m_autoConnect;
    QString m_accountId;
    QVariantHash m_credentials;
    QVariantHash m_configuration;
    QVariantMap m_acl;
    QStringList m_types;
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

    virtual Account* createAccount( const QString& pluginId = QString() ) = 0;
};

};

};

Q_DECLARE_INTERFACE( Tomahawk::Accounts::AccountFactory, "tomahawk.AccountFactory/1.0" )

#endif