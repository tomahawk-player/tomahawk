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

#include <QObject>
#include <QString>
#include <QHash>
#include <QTimer>
#include <QMutex>
#include <QVariant>
#include <QQueue>
#include <QStringList>
#include <QUuid>

#include "HeadlessCheck.h"
#include "DllMacro.h"

#define ACLUSERVERSION 1

class DLLEXPORT ACLRegistry : public QObject
{
    Q_OBJECT

public:

    static ACLRegistry* instance();
    static void setInstance( ACLRegistry* instance );

    enum ACL {
        NotFound = 0,
        Deny = 1,
        Read = 2,
        Stream = 3
    };

    struct User {
        QString uuid;
        QString friendlyName;
        QStringList knownDbids;
        QStringList knownAccountIds;
        ACLRegistry::ACL acl;

        User()
            : uuid( QUuid::createUuid().toString() )
            , friendlyName()
            , knownDbids()
            , knownAccountIds()
            , acl( ACLRegistry::NotFound )
            {}

        ~User()
            {}

        User( QString p_uuid, QString p_friendlyName, QStringList p_knownDbids, QStringList p_knownAccountIds, ACLRegistry::ACL p_acl )
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

signals:
    void aclResult( QString nodeid, QString username, ACLRegistry::ACL peerStatus );

public slots:
    /**
     * @brief Checks if peer is authorized; optionally, can authorize peer with given type if not found
     *
     * @param dbid DBID of peer
     * @param globalType Global ACL to store if peer not found; if ACLRegistry::NotFound, does not store the peer Defaults to ACLRegistry::NotFound.
     * @param username If not empty, will store the given username along with the new ACL value. Defaults to QString().
     * @return ACLRegistry::ACL
     **/
    virtual ACLRegistry::ACL isAuthorizedUser( const QString &dbid, const QString &username, ACLRegistry::ACL globalType = ACLRegistry::NotFound, bool skipEmission = false ) = 0;
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
};

Q_DECLARE_METATYPE( ACLRegistry::ACL );
Q_DECLARE_METATYPE( ACLRegistry::User );

#endif // TOMAHAWK_ACLREGISTRY_H
