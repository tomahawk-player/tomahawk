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

#include "LocalConfigStorage.h"

#include "Account.h"
#include "AccountManager.h"
#include "CredentialsManager.h"
#include "utils/Logger.h"

namespace Tomahawk
{

namespace Accounts
{

const QString LocalConfigStorage::s_credentialsServiceName = "Tomahawk";

#ifdef Q_OS_MAC
QString
LocalConfigStorage::credentialsServiceName()
{
    return s_credentialsServiceName;
}
#endif

LocalConfigStorage::LocalConfigStorage( QObject* parent )
    : ConfigStorage( parent )
{
    m_accountIds = TomahawkSettings::instance()->accounts();
}


void
LocalConfigStorage::init()
{
    // tell CredentialsManager which account ids it will be writing credentials for and in which svc

    CredentialsManager* cm = AccountManager::instance()->credentialsManager();
    connect( cm, SIGNAL( serviceReady( QString ) ),
             this, SLOT( onCredentialsManagerReady( QString ) ) );
    AccountManager::instance()->credentialsManager()->addService( s_credentialsServiceName,
                                                                  m_accountIds );

    tDebug() << Q_FUNC_INFO << "LOADING ALL CREDENTIALS FOR SERVICE" << s_credentialsServiceName << m_accountIds;
}


QString
LocalConfigStorage::id() const
{
    return "localconfigstorage";
}


void
LocalConfigStorage::onCredentialsManagerReady( const QString& service )
{
    if ( service != s_credentialsServiceName )
        return;

    //no need to listen for it any more
    disconnect( this, SLOT( onCredentialsManagerReady( QString ) ) );

    tDebug() << Q_FUNC_INFO << "CredentialsManager is now ready for service" << service
             << "with keys" << AccountManager::instance()->credentialsManager()->keys( service );
    emit ready();
}


QStringList
LocalConfigStorage::accountIds() const
{
    return m_accountIds;
}


unsigned int
LocalConfigStorage::priority() const
{
    return 0;
}


void
LocalConfigStorage::deduplicateFrom( const ConfigStorage* other)
{
    Q_UNUSED( other )
    // I'm priority 0 so I don't have to deduplicate anything
}


void
LocalConfigStorage::save( const QString& accountId, const Account::Configuration& cfg )
{
    tDebug() << Q_FUNC_INFO << "about to save configuration for" << accountId;
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "accounts/" + accountId );
    s->setValue( "accountfriendlyname", cfg.accountFriendlyName );
    s->setValue( "enabled", cfg.enabled );
    s->setValue( "configuration", cfg.configuration );
    s->setValue( "acl", cfg.acl );
    s->setValue( "types", cfg.types );
    s->endGroup();
    s->sync();

    CredentialsManager* c = AccountManager::instance()->credentialsManager();
    c->setCredentials( s_credentialsServiceName, accountId, cfg.credentials );

    if ( !m_accountIds.contains( accountId ) )
        m_accountIds.append( accountId );
}


void
LocalConfigStorage::load( const QString& accountId, Account::Configuration& cfg ) const
{
    tDebug() << Q_FUNC_INFO << "about to load creds for" << accountId << "- assuming CredentialsManager is *ready*.";
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "accounts/" + accountId );
    cfg.accountFriendlyName = s->value( "accountfriendlyname", QString() ).toString();
    cfg.enabled =             s->value( "enabled", false ).toBool();
    cfg.configuration =       s->value( "configuration", QVariantHash() ).toHash();
    cfg.acl =                 s->value( "acl", QVariantMap() ).toMap();
    cfg.types =               s->value( "types", QStringList() ).toStringList();
    s->endGroup();

    CredentialsManager* c = AccountManager::instance()->credentialsManager();
    QVariant credentials = c->credentials( s_credentialsServiceName, accountId );
    if ( credentials.type() == QVariant::Hash )
        cfg.credentials = credentials.toHash();
}


void
LocalConfigStorage::remove( const QString& accountId )
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
    c->setCredentials( s_credentialsServiceName, accountId, QVariantHash() );
}

}
}
