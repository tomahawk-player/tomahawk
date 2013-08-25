/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TOMAHAWK_ACLREGISTRY_H
#define TOMAHAWK_ACLREGISTRY_H

#include "network/acl/AclRequest.h"
#include "utils/WeakObjectList.h"

#include "DllMacro.h"
#include "Typedefs.h"

#include <QObject>
#include <QString>
#include <QHash>
#include <QTimer>
#include <QMutex>
#include <QVariant>
#include <QQueue>
#include <QStringList>
#include <QUuid>

#define ACLUSERVERSION 1

class DLLEXPORT ACLRegistry : public QObject
{
    Q_OBJECT

public:

    static ACLRegistry* instance();
    static void setInstance( ACLRegistry* instance );

    struct User {
        QString uuid;
        QString friendlyName;
        QStringList knownDbids;
        QStringList knownAccountIds;
        Tomahawk::ACLStatus::Type acl;

        User()
            : uuid( QUuid::createUuid().toString() )
            , friendlyName()
            , knownDbids()
            , knownAccountIds()
            , acl( Tomahawk::ACLStatus::NotFound )
            {}

        ~User()
            {}

        User( QString p_uuid, QString p_friendlyName, QStringList p_knownDbids, QStringList p_knownAccountIds, Tomahawk::ACLStatus::Type p_acl )
            : uuid( p_uuid )
            , friendlyName( p_friendlyName )
            , knownDbids( p_knownDbids )
            , knownAccountIds( p_knownAccountIds )
            , acl( p_acl )
            {}

        User( const User &other )
            : uuid( other.uuid )
            , friendlyName( other.friendlyName )
            , knownDbids( other.knownDbids )
            , knownAccountIds( other.knownAccountIds )
            , acl( other.acl )
            {}
    };

    ACLRegistry( QObject *parent = 0 );
    virtual ~ACLRegistry();

    /**
     * @brief Check if the request is authorized to access via our instance via network.
     *
     * Checking is done asynchronously and will not block the calling thread. This function
     * can be called from any thread. It will ensure itself that the underlying logic is run
     * in the correct thread.
     *
     * @param request The connection request which should be authorized.
     */
    virtual void isAuthorizedRequest( const Tomahawk::Network::ACL::aclrequest_ptr& request );

signals:
    void aclResult( QString nodeid, QString username, Tomahawk::ACLStatus::Type peerStatus );

public slots:
    /**
     * @brief Checks if peer is authorized; optionally, can authorize peer with given type if not found
     *
     * @param dbid DBID of peer
     * @param globalType Global ACL to store if peer not found; if ACLRegistry::NotFound, does not store the peer Defaults to ACLRegistry::NotFound.
     * @param username If not empty, will store the given username along with the new ACL value. Defaults to QString().
     * @return Tomahawk::ACLStatus::Type
     **/
    virtual Tomahawk::ACLStatus::Type isAuthorizedUser( const QString &dbid, const QString &username, Tomahawk::ACLStatus::Type globalType = Tomahawk::ACLStatus::NotFound, bool skipEmission = false ) = 0;
    virtual void wipeEntries();

protected:
    /**
     * @brief Saves the cache.
     *
     * @return void
     **/
    virtual void save();
    virtual void load();

    QList< ACLRegistry::User > m_cache;

    static ACLRegistry* s_instance;

private slots:
    void aclResultForRequest( QString nodeid, QString username, Tomahawk::ACLStatus::Type peerStatus );

private:
    Tomahawk::Utils::WeakObjectList<Tomahawk::Network::ACL::AclRequest> m_aclRequests;
};

Q_DECLARE_METATYPE( ACLRegistry::User )

#endif // TOMAHAWK_ACLREGISTRY_H
