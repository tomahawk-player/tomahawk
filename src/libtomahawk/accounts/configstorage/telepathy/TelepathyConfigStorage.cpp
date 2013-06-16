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

#include <TelepathyQt/Account>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/AccountSet>

#include <QTimer>



//NOTE: Both Tomahawk::Accounts and Tp have class names Account and AccountManager.


Tomahawk::Accounts::TelepathyConfigStorage::TelepathyConfigStorage( QObject* parent )
    : Tomahawk::Accounts::ConfigStorage( parent )
    , m_credentialsServiceName( "telepathy-kde" )
{
    tDebug() << Q_FUNC_INFO;
}


void
Tomahawk::Accounts::TelepathyConfigStorage::init()
{

    m_tpam = Tp::AccountManager::create();
    connect( m_tpam->becomeReady(), SIGNAL( finished( Tp::PendingOperation* ) ),
             this, SLOT( onTpAccountManagerReady( Tp::PendingOperation* ) ) );
}


void
Tomahawk::Accounts::TelepathyConfigStorage::onTpAccountManagerReady( Tp::PendingOperation* op )
{
    if ( op->isError() )
    {
        tDebug() << "Telepathy AccountManager cannot become ready:"
                 << op->errorName() << "-" << op->errorMessage();
        emit ready(); //we bail, this CS is ready to provide 0 accounts
        return;
    }

    QStringList keychainIds;
    foreach ( const Tp::AccountPtr& acc, m_tpam->validAccounts()->accounts() )
    {
        if ( acc->protocolName() == "jabber" )
        {
            m_accountIds << telepathyPathToAccountId( acc->objectPath() );
            keychainIds << acc->uniqueIdentifier();
        }
    }

    // tell CredentialsManager which account ids it will be writing credentials for and in which svc

    CredentialsManager* cm = AccountManager::instance()->credentialsManager();
    connect( cm, SIGNAL( serviceReady( QString ) ),
             this, SLOT( onCredentialsManagerReady( QString ) ) );
    Tomahawk::Accounts::AccountManager::instance()->credentialsManager()->addService( m_credentialsServiceName,
                                                                                      keychainIds );
    tDebug() << Q_FUNC_INFO << "LOADING ALL CREDENTIALS FOR SERVICE" << m_credentialsServiceName << m_accountIds << keychainIds;
}


void
Tomahawk::Accounts::TelepathyConfigStorage::onCredentialsManagerReady( const QString& service )
{
    if ( service != m_credentialsServiceName )
        return;

    //no need to listen for it any more
    disconnect( this, SLOT( onCredentialsManagerReady( QString ) ) );

    emit ready();
}


QString
Tomahawk::Accounts::TelepathyConfigStorage::telepathyPathToAccountId( const QString& objectPath )
{
    return QString( "xmppaccount_" ) + objectPath;
}


QString
Tomahawk::Accounts::TelepathyConfigStorage::accountIdToTelepathyPath( const QString& accountId )
{
    QString prefix( "xmppaccount_" );
    QString r = accountId;
    if ( r.startsWith( prefix ) )
        r.remove( 0, prefix.length() );
    return r;
}


QStringList
Tomahawk::Accounts::TelepathyConfigStorage::accountIds() const
{
    return m_accountIds;
}


void
Tomahawk::Accounts::TelepathyConfigStorage::save( const QString& accountId, const Account::Configuration& cfg )
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
Tomahawk::Accounts::TelepathyConfigStorage::load( const QString& accountId, Account::Configuration& cfg )
{
//    TomahawkSettings* s = TomahawkSettings::instance();
//    s->beginGroup( "accounts/" + accountId );
//    cfg.accountFriendlyName = s->value( "accountfriendlyname", QString() ).toString();
//    cfg.enabled =             s->value( "enabled", false ).toBool();
//    cfg.configuration =       s->value( "configuration", QVariantHash() ).toHash();
//    cfg.acl =                 s->value( "acl", QVariantMap() ).toMap();
//    cfg.types =               s->value( "types", QStringList() ).toStringList();
//    s->endGroup();

    Tp::AccountPtr account = m_tpam->accountForObjectPath( accountIdToTelepathyPath( accountId ) );

    cfg.accountFriendlyName = account->normalizedName();

    cfg.enabled = true;
    cfg.acl = QVariantMap();

    QStringList types;
    types << "SipType";
    cfg.types = types;

    if ( !account->parameters()[ "port" ].isNull() )
        cfg.configuration[ "port" ] = account->parameters()[ "port" ].toString();
    else
        cfg.configuration[ "port" ] = "5222";

    if ( !account->parameters()[ "server" ].isNull() )
        cfg.configuration[ "server" ] = account->parameters()[ "server" ].toString();

    if ( !account->parameters()[ "require-encryption" ].isNull() )
        cfg.configuration[ "enforcesecure" ] = account->parameters()[ "require-encryption" ].toBool();

    cfg.configuration[ "publishtracks" ] = true;

    Tomahawk::Accounts::CredentialsManager* c = Tomahawk::Accounts::AccountManager::instance()->credentialsManager();
    cfg.credentials = QVariantHash();

    if ( !account->parameters()[ "account" ].isNull() )
        cfg.credentials[ "username" ] = account->parameters()[ "account" ].toString();
    else
        cfg.credentials[ "username" ] = account->normalizedName();

    QVariant credentials = c->credentials( m_credentialsServiceName, account->uniqueIdentifier() );
    if ( credentials.type() == QVariant::String )
        cfg.credentials[ "password" ] = credentials.toString();
}


void
Tomahawk::Accounts::TelepathyConfigStorage::remove( const QString& accountId )
{
//    TomahawkSettings* s = TomahawkSettings::instance();
//    s->beginGroup( "accounts/" + accountId );
//    s->remove( "accountfriendlyname" );
//    s->remove( "enabled" );
//    s->remove( "configuration" );
//    s->remove( "acl" );
//    s->remove( "types" );
//    s->endGroup();
//    s->remove( "accounts/" + accountId );

//    Tomahawk::Accounts::CredentialsManager* c = Tomahawk::Accounts::AccountManager::instance()->credentialsManager();
//    c->setCredentials( m_credentialsServiceName, account->uniqueIdentifier(), QVariantHash() );
}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::ConfigStorage, Tomahawk::Accounts::TelepathyConfigStorage )
