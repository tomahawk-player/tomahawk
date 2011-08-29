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

#include "typedefs.h"
#include "dllmacro.h"
#include "infosystem/infosystem.h"
#include "sip/SipPlugin.h"

namespace Tomahawk
{

class DLLEXPORT Account
{

    typedef QMap< QString, bool > ACLMap;
    
public:
    enum AccountTypes { InfoType, SipType };

    explicit Account();
    virtual ~Account();

    QString accountServiceName(); // e.g. "Twitter", "Last.fm"
    void setAccountServiceName( const QString &serviceName );

    QString accountFriendlyName(); // e.g. screen name on the service, JID, etc.
    void setAccountFriendlyName( const QString &friendlyName );
    
    bool autoConnect();
    void setAutoConnect( bool autoConnect );
    
    QStringMap credentials();
    void setCredentials( const QStringMap &credentialMap );
    
    QVariantMap configuration();
    void setConfiguration( const QVariantMap &configuration );
    QWidget* configurationWidget();

    ACLMap acl();
    void setAcl( const ACLMap &acl );
    QWidget* aclWidget();

    QSet< AccountTypes > types();
    void setTypes( const QSet< AccountTypes > types );

    Tomahawk::InfoSystem::InfoPlugin* infoPlugin();
    SipPlugin* sipPlugin();
};

};

#endif // ACCOUNT_H
