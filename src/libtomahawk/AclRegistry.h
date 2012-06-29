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

class DLLEXPORT AclRegistry : public QObject
{
    Q_OBJECT

public:

    static AclRegistry* instance();
    static void setInstance( AclRegistry* instance );

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
        AclRegistry::ACL acl;

        User()
            : uuid( QUuid::createUuid().toString() )
            , friendlyName()
            , knownDbids()
            , knownAccountIds()
            , acl( AclRegistry::NotFound )
            {}

        ~User()
            {}

        User( QString p_uuid, QString p_friendlyName, QStringList p_knownDbids, QStringList p_knownAccountIds, AclRegistry::ACL p_acl )
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

    AclRegistry( QObject *parent = 0 );
    virtual ~AclRegistry();

signals:
    void aclResult( QString nodeid, QString username, AclRegistry::ACL peerStatus );

public slots:
    /**
     * @brief Checks if peer is authorized; optionally, can authorize peer with given type if not found
     *
     * @param dbid DBID of peer
     * @param globalType Global ACL to store if peer not found; if AclRegistry::NotFound, does not store the peer Defaults to AclRegistry::NotFound.
     * @param username If not empty, will store the given username along with the new ACL value. Defaults to QString().
     * @return AclRegistry::ACL
     **/
    virtual AclRegistry::ACL isAuthorizedUser( const QString &dbid, const QString &username, AclRegistry::ACL globalType = AclRegistry::NotFound, bool skipEmission = false ) = 0;
    virtual void wipeEntries();

protected:
    /**
     * @brief Saves the cache.
     *
     * @return void
     **/
    virtual void save();
    virtual void load();

    QList< AclRegistry::User > m_cache;

    static AclRegistry* s_instance;
};

Q_DECLARE_METATYPE( AclRegistry::ACL );
Q_DECLARE_METATYPE( AclRegistry::User );

#endif // TOMAHAWK_ACLREGISTRY_H
