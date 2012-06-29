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

class AclJobItem;

class AclRegistryImpl : public AclRegistry
{
    Q_OBJECT

public:

    AclRegistryImpl( QObject *parent = 0 );
    virtual ~AclRegistryImpl();

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
    virtual AclRegistry::ACL isAuthorizedUser( const QString &dbid, const QString &username, AclRegistry::ACL globalType = AclRegistry::NotFound, bool skipEmission = false );

#ifndef ENABLE_HEADLESS
    void getUserDecision( AclRegistry::User user, const QString &username );

    virtual void wipeEntries();

private slots:
    void userDecision( AclRegistry::User user );
    void queueNextJob();
#endif

private:
    QQueue< AclJobItem* > m_jobQueue;
    int m_jobCount;
};

#endif // TOMAHAWK_ACLREGISTRYIMPL_H
