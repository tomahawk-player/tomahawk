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

#ifndef TOMAHAWK_ACLREGISTRYIMPL_H
#define TOMAHAWK_ACLREGISTRYIMPL_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QTimer>
#include <QMutex>
#include <QVariant>
#include <QQueue>
#include <QStringList>
#include <QUuid>

#include "AclRegistry.h"
#include "HeadlessCheck.h"
#include "DllMacro.h"

class ACLJobItem;

class ACLRegistryImpl : public ACLRegistry
{
    Q_OBJECT

public:

    ACLRegistryImpl( QObject *parent = 0 );
    virtual ~ACLRegistryImpl();

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
    virtual ACLRegistry::ACL isAuthorizedUser( const QString &dbid, const QString &username, ACLRegistry::ACL globalType = ACLRegistry::NotFound, bool skipEmission = false );
    virtual void wipeEntries();
    
protected:
    virtual void load();
    virtual void save();
    
#ifndef ENABLE_HEADLESS
    void getUserDecision( ACLRegistry::User user, const QString &username );

private slots:
    void userDecision( ACLRegistry::User user );
    void queueNextJob();
#endif

private:
    QQueue< ACLJobItem* > m_jobQueue;
    int m_jobCount;
};

#endif // TOMAHAWK_ACLREGISTRYIMPL_H
