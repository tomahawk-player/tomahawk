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
#include <tomahawksettings.h>

namespace Tomahawk
{

namespace Accounts
{

typedef QMap< QString, bool > ACLMap;
    
enum AccountType { InfoType, SipType };

class DLLEXPORT Account : public QObject
{

    Q_OBJECT
    
public:
    explicit Account()
        : QObject()
        {
            m_autoConnect = false;
        }
    virtual ~Account();

    QString accountServiceName() const; // e.g. "Twitter", "Last.fm"
    void setAccountServiceName( const QString &serviceName );

    QString accountFriendlyName() const; // e.g. screen name on the service, JID, etc.
    void setAccountFriendlyName( const QString &friendlyName );
    
    bool autoConnect() const { return m_autoConnect; }
    void setAutoConnect( bool autoConnect ) { m_autoConnect = autoConnect; }
    
    QHash< QString, QString > credentials() { return m_credentials; }
    void setCredentials( const QHash< QString, QString > &credentialMap );

    QIcon icon() const;
    
    QVariantMap configuration() const;
    void setConfiguration( const QVariantMap &configuration );
    QWidget* configurationWidget();

    ACLMap acl() const;
    void setAcl( const ACLMap &acl );
    QWidget* aclWidget();

    QSet< AccountType > types() const;
    void setTypes( const QSet< AccountType > types );

    Tomahawk::InfoSystem::InfoPlugin* infoPlugin();
    SipPlugin* sipPlugin();

private:
    bool m_autoConnect;
    QHash< QString, QString > m_credentials;
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

protected:
    QString generateId()
    {
        QString uniq = QUuid::createUuid().toString().mid( 1, 8 );
        return factoryId() + "_" + uniq;
    }
};

};

};

Q_DECLARE_INTERFACE( Tomahawk::Accounts::AccountFactory, "tomahawk.AccountFactory/1.0" )

#endif