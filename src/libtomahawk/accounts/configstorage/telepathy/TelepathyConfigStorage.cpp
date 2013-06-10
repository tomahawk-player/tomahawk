/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "TelepathyConfigStorage.h"

#include "accounts/Account.h"
#include "accounts/AccountManager.h"
#include "accounts/CredentialsManager.h"
#include "utils/Logger.h"


namespace Tomahawk
{

namespace Accounts
{


TelepathyConfigStorage::TelepathyConfigStorage( QObject* parent )
    : ConfigStorage( parent )
    , m_credentialsServiceName( "telepathy-kde" )
{
    tDebug() << Q_FUNC_INFO;


    // tell CredentialsManager which account ids it will be writing credentials for and in which svc
    // so it can preload them when we call CM::loadCredentials()
    AccountManager::instance()->credentialsManager()->addService( m_credentialsServiceName,
                                                                  m_accountIds );
}


QStringList
TelepathyConfigStorage::accountIds() const
{
    return m_accountIds;
}


void
TelepathyConfigStorage::save( const QString& accountId, const Account::Configuration& cfg )
{
//    TomahawkSettings* s = TomahawkSettings::instance();
//    s->beginGroup( "accounts/" + accountId );
//    s->setValue( "accountfriendlyname", cfg.accountFriendlyName );
//    s->setValue( "enabled", cfg.enabled );
//    s->setValue( "configuration", cfg.configuration );
//    s->setValue( "acl", cfg.acl );
//    s->setValue( "types", cfg.types );
//    s->endGroup();
//    s->sync();

    //CredentialsManager* c = AccountManager::instance()->credentialsManager();
    //c->setCredentials( m_credentialsServiceName, accountId, cfg.credentials );

    if ( !m_accountIds.contains( accountId ) )
        m_accountIds.append( accountId );
}


void
TelepathyConfigStorage::load( const QString& accountId, Account::Configuration& cfg )
{
//    TomahawkSettings* s = TomahawkSettings::instance();
//    s->beginGroup( "accounts/" + accountId );
//    cfg.accountFriendlyName = s->value( "accountfriendlyname", QString() ).toString();
//    cfg.enabled =             s->value( "enabled", false ).toBool();
//    cfg.configuration =       s->value( "configuration", QVariantHash() ).toHash();
//    cfg.acl =                 s->value( "acl", QVariantMap() ).toMap();
//    cfg.types =               s->value( "types", QStringList() ).toStringList();
//    s->endGroup();

    CredentialsManager* c = AccountManager::instance()->credentialsManager();
    QString prefix( "tomahawkaccount_" );
    QString idInKeychain = accountId;
    if ( idInKeychain.startsWith( prefix ) )
        idInKeychain.remove( 0, prefix.length() );
    cfg.credentials =         c->credentials( m_credentialsServiceName, idInKeychain );
}


void
TelepathyConfigStorage::remove( const QString& accountId )
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "accounts/" + accountId );
    s->remove( "accountfriendlyname" );
    s->remove( "enabled" );
    s->remove( "configuration" );
    s->remove( "acl" );
    s->remove( "types" );
    s->endGroup();
    s->remove( "accounts/" + accountId );

    CredentialsManager* c = AccountManager::instance()->credentialsManager();
    c->setCredentials( m_credentialsServiceName, accountId, QVariantHash() );
}

}
}
