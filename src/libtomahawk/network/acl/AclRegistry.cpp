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

#include "AclRegistry.h"

#include <QThread>
#include <QVariant>

#include "utils/Logger.h"


QDataStream& operator<<( QDataStream &out, const ACLRegistry::User &user )
{
    out << ACLUSERVERSION;
    out << user.uuid;
    out << user.friendlyName;
    out << user.knownDbids.length();
    foreach( QString knownDbid, user.knownDbids )
        out << knownDbid;
    out << user.knownAccountIds.length();
    foreach( QString knownAccount, user.knownAccountIds )
        out << knownAccount;
    out << (int)( user.acl );
    return out;
}

QDataStream& operator>>( QDataStream &in, ACLRegistry::User &user )
{
    int ver;
    in >> ver;
    if ( ver == ACLUSERVERSION )
    {
        in >> user.uuid;
        in >> user.friendlyName;
        int dbidsLength;
        in >> dbidsLength;
        QString knownDbid;
        for ( int i = 0; i < dbidsLength; i++ )
        {
            in >> knownDbid;
            user.knownDbids << knownDbid;
        }
        int accountsLength;
        in >> accountsLength;
        QString knownAccountId;
        for ( int i = 0; i < accountsLength; i++ )
        {
            in >> knownAccountId;
            user.knownAccountIds << knownAccountId;
        }
        int aclIn;
        in >> aclIn;
        user.acl = (Tomahawk::ACLStatus::Type)( aclIn );
    }
    return in;
}


ACLRegistry* ACLRegistry::s_instance = 0;

ACLRegistry*
ACLRegistry::instance()
{
    return s_instance;
}


void
ACLRegistry::setInstance( ACLRegistry* instance )
{
    s_instance = instance;
}


ACLRegistry::ACLRegistry( QObject* parent )
    : QObject( parent )
{
    qRegisterMetaType< Tomahawk::ACLStatus::Type >( "Tomahawk::ACLStatus::Type" );
    qRegisterMetaType< ACLRegistry::User >( "ACLRegistry::User" );
    qRegisterMetaTypeStreamOperators< ACLRegistry::User >( "ACLRegistry::User" );

    connect( this, SIGNAL( aclResult( QString, QString, Tomahawk::ACLStatus::Type ) ),
             SLOT( aclResultForRequest(QString,QString,Tomahawk::ACLStatus::Type ) ) );
}


ACLRegistry::~ACLRegistry()
{
}


void
ACLRegistry::isAuthorizedRequest( const Tomahawk::Network::ACL::aclrequest_ptr& request )
{
    m_aclRequests.insert( request );
    // Ensure that we calling the registry in its own function and do not block the caller thread.
    QMetaObject::invokeMethod( this, "isAuthorizedUser", Qt::QueuedConnection,
                               Q_ARG( QString, request->nodeid() ),
                               Q_ARG( QString, request->username() ),
                               Q_ARG( Tomahawk::ACLStatus::Type, request->status() ) );
}


void
ACLRegistry::load()
{
}

void
ACLRegistry::aclResultForRequest( QString nodeid, QString username, Tomahawk::ACLStatus::Type peerStatus )
{
    QMutableListIterator<Tomahawk::Network::ACL::aclrequest_wptr> iter = m_aclRequests.iter();
    while ( iter.hasNext() )
    {
        Tomahawk::Network::ACL::aclrequest_wptr wptr = iter.next();

        // Remove dangling objects
        if ( wptr.isNull() )
        {
            iter.remove();
            continue;
        }

        // Try to (greedy) match all possible AclRequests
        Tomahawk::Network::ACL::aclrequest_ptr request = wptr.toStrongRef();
        if ( request->nodeid() == nodeid && request->username() == username )
        {
            QMetaObject::invokeMethod( request.data(), "emitDecision", Q_ARG( Tomahawk::ACLStatus::Type, peerStatus ) );
            // We made a decision, so strip this request from the queue
            iter.remove();
        }
    }
}


void
ACLRegistry::save()
{
}


void
ACLRegistry::wipeEntries()
{
    tLog() << Q_FUNC_INFO;
    m_cache.clear();
}

