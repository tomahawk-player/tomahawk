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

#include "TomahawkSettings.h"
#include "TomahawkApp.h"
#include "Source.h"

#include "utils/Logger.h"


QDataStream& operator<<( QDataStream &out, const AclRegistry::User &user )
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

QDataStream& operator>>( QDataStream &in, AclRegistry::User &user )
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
        user.acl = (AclRegistry::ACL)( aclIn );
    }
    return in;
}


AclRegistry* AclRegistry::s_instance = 0;

AclRegistry*
AclRegistry::instance()
{
    return s_instance;
}


void
AclRegistry::setInstance( AclRegistry* instance )
{
    s_instance = instance;
}


AclRegistry::AclRegistry( QObject* parent )
    : QObject( parent )
{
    qRegisterMetaType< AclRegistry::ACL >( "AclRegistry::ACL" );
    qRegisterMetaType< AclRegistry::User >( "AclRegistry::User" );
    qRegisterMetaTypeStreamOperators< AclRegistry::User >( "AclRegistry::User" );
}


AclRegistry::~AclRegistry()
{
}


void
AclRegistry::load()
{
    tLog() << Q_FUNC_INFO;
    QVariantList entryList = TomahawkSettings::instance()->aclEntries();
    foreach ( QVariant entry, entryList )
    {
        if ( !entry.isValid() || !entry.canConvert< AclRegistry::User >() )
        {
            tLog() << Q_FUNC_INFO << "entry is invalid";
            continue;
        }
        tLog() << Q_FUNC_INFO << "loading entry";
        AclRegistry::User entryUser = entry.value< AclRegistry::User >();
        if ( entryUser.knownAccountIds.empty() || entryUser.knownDbids.empty() )
        {
            tLog() << Q_FUNC_INFO << "user known account/dbids is empty";
            continue;
        }
        m_cache.append( entryUser );
    }
}


void
AclRegistry::save()
{
    tLog() << Q_FUNC_INFO;
    QVariantList entryList;
    foreach ( AclRegistry::User user, m_cache )
    {
        tLog() << Q_FUNC_INFO << "user is " << user.uuid << " with known name " << user.knownAccountIds.first();
        QVariant val = QVariant::fromValue< AclRegistry::User >( user );
        if ( val.isValid() )
            entryList.append( val );
    }
    TomahawkSettings::instance()->setAclEntries( entryList );
}


void
AclRegistry::wipeEntries()
{
    tLog() << Q_FUNC_INFO;
    m_cache.clear();
}

