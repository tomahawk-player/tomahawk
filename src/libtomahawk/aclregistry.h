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

#include "headlesscheck.h"
#include "dllmacro.h"

class DLLEXPORT ACLRegistry : public QObject
{
    Q_OBJECT

public:

    static ACLRegistry* instance();

    enum ACL {
        Allow = 0,
        Deny = 1,
        NotFound = 2,
        AllowOnce = 3,
        DenyOnce = 4
    };

    ACLRegistry( QObject *parent = 0 );
    ~ACLRegistry();
    
    /**
     * @brief Registers the global ACL value for this peer
     *
     * @param dbid DBID of peer
     * @param globalType Global ACL to use for this peer. ACLRegistry::NotFound is invalid and will return immediately.
     * @param username If not empty, will store the given username along with the new ACL value. Defaults to QString().
     * @return void
     **/
    void registerPeer( const QString &dbid, ACLRegistry::ACL globalType, const QString &username = QString() );

    /**
     * @brief Checks if peer is authorized, using the username. Optionally, can change authorization of the peer, but only if the peer is found.
     *
     * @param username Username for the peer
     * @param globalType Global ACL to store if peer is found; if ACLRegistry::NotFound, does not change the ACL. Defaults to ACLRegistry::NotFound.
     * @return QPair< QString, ACLRegistry::ACL >
     **/
    QPair< QString, ACLRegistry::ACL > isAuthorizedUser( const QString &username, ACLRegistry::ACL globalType = ACLRegistry::NotFound );

    /**
     * @brief Registers an alias for a known peer. If you do not know the DBID, you can retrieve it via isAuthorizedUser first.
     *
     * @param dbid DBID of peer
     * @param username Username of the peer to be added to the entry
     * @return void
     **/
    void registerAlias( const QString &dbid, const QString &username );

signals:
    void aclResult( QString nodeid, ACLRegistry::ACL peerStatus );
    
public slots:
    /**
     * @brief Checks if peer is authorized; optionally, can authorize peer with given type if not found
     *
     * @param dbid DBID of peer
     * @param globalType Global ACL to store if peer not found; if ACLRegistry::NotFound, does not store the peer Defaults to ACLRegistry::NotFound.
     * @param username If not empty, will store the given username along with the new ACL value. Defaults to QString().
     * @return ACLRegistry::ACL
     **/
    void isAuthorizedPeer( const QString &dbid, ACLRegistry::ACL globalType = ACLRegistry::NotFound, const QString &username = QString() );

    
#ifndef ENABLE_HEADLESS
    ACLRegistry::ACL getUserDecision( const QString &username );
#endif
    
//     ACLRegistry::ACL isAuthorizedPath( const QString &dbid, const QString &path );
//     void authorizePath( const QString &dbid, const QString &path, ACLRegistry::ACL type );

private:
    /**
     * @brief Saves the cache.
     *
     * @return void
     **/
    void save();

    QVariantHash m_cache;

    static ACLRegistry* s_instance;
};

#endif // TOMAHAWK_ACLREGISTRY_H
