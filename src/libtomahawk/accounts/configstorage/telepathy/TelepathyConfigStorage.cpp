/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "TelepathyConfigStorageConfigWidgetPlugin.h"


#include "accounts/Account.h"
#include "accounts/AccountManager.h"
#include "accounts/CredentialsManager.h"
#include "utils/Logger.h"
#include "utils/PluginLoader.h"
#include "utils/TomahawkUtilsGui.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/AccountSet>

#include <QDialog>
#include <QDialogButtonBox>
#include <QProcess>
#include <QTimer>
#include <QBoxLayout>



//NOTE: Both Tomahawk::Accounts and Tp have class names Account and AccountManager.


Tomahawk::Accounts::TelepathyConfigStorage::TelepathyConfigStorage( QObject* parent )
    : Tomahawk::Accounts::ConfigStorage( parent )
    , m_credentialsServiceName( "telepathy-kde" )
{
    tDebug() << Q_FUNC_INFO;
    m_allowedPrefixes << "xmppaccount_"
                      << "googleaccount_";
    loadConfigWidgetPlugins();
}


void
Tomahawk::Accounts::TelepathyConfigStorage::init()
{

    m_tpam = Tp::AccountManager::create();
    connect( m_tpam->becomeReady(), SIGNAL( finished( Tp::PendingOperation* ) ),
             this, SLOT( onTpAccountManagerReady( Tp::PendingOperation* ) ) );
}


QString
Tomahawk::Accounts::TelepathyConfigStorage::id() const
{
    return "telepathyconfigstorage";
}


QString
Tomahawk::Accounts::TelepathyConfigStorage::prettyName() const
{
    return tr( "the KDE instant messaging framework" );
}


QPixmap
Tomahawk::Accounts::TelepathyConfigStorage::icon() const
{
    return QPixmap( ":/telepathy/kde.png" );
}


bool
Tomahawk::Accounts::TelepathyConfigStorage::execConfigDialog( QWidget* parent )
{
    if ( !m_configWidgetPlugins.isEmpty() )
    {
        QDialog dialog( parent );
        dialog.setWindowTitle( tr( "KDE Instant Messaging Accounts" ) );
        dialog.resize( parent->logicalDpiX() * 3.0, parent->logicalDpiY() * 2.2 );
        dialog.setLayout( new QVBoxLayout );
        dialog.layout()->addWidget( m_configWidgetPlugins.first()->configWidget() );

        QDialogButtonBox* box = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal );
        dialog.layout()->addWidget( box );
        connect( box, SIGNAL( clicked( QAbstractButton* ) ), &dialog, SLOT( accept() ) );
        return dialog.exec();
    }

    return false;
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
            m_accountIds << telepathyPathToAccountId( acc->objectPath(), acc->serviceName() );
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


void
Tomahawk::Accounts::TelepathyConfigStorage::loadConfigWidgetPlugins()
{
    tDebug() << Q_FUNC_INFO;
    foreach( QObject* plugin, Tomahawk::Utils::PluginLoader( "configstorage_telepathy" ).loadPlugins().values() )
    {
        TelepathyConfigStorageConfigWidgetPlugin* configWidgetPlugin = qobject_cast< TelepathyConfigStorageConfigWidgetPlugin* >( plugin );
        if( !configWidgetPlugin )
        {
            tLog() << "Tried to load invalid TelepathyConfigStorageConfigWidgetPlugin";
            continue;
        }

        m_configWidgetPlugins << configWidgetPlugin;
    }
}


QString
Tomahawk::Accounts::TelepathyConfigStorage::telepathyPathToAccountId( const QString& objectPath, const QString& telepathyServiceName )
{
    if ( telepathyServiceName == "google-talk" )
        return QString( "googleaccount_" ) + objectPath;
    return QString( "xmppaccount_" ) + objectPath;
}


QString
Tomahawk::Accounts::TelepathyConfigStorage::accountIdToTelepathyPath( const QString& accountId ) const
{
    QString r = accountId;
    foreach ( QString prefix, m_allowedPrefixes )
    {
        if ( r.startsWith( prefix ) )
            r.remove( 0, prefix.length() );
    }
    return r;
}


QStringList
Tomahawk::Accounts::TelepathyConfigStorage::accountIds() const
{
    return m_accountIds;
}


unsigned int
Tomahawk::Accounts::TelepathyConfigStorage::priority() const
{
    return 30;
}


void
Tomahawk::Accounts::TelepathyConfigStorage::deduplicateFrom( const Tomahawk::Accounts::ConfigStorage* other )
{
    tDebug() << "MY    ACCOUNTS:" << accountIds();
    tDebug() << "OTHER ACCOUNTS:" << other->accountIds();

    QSet< QString > toDelete;
    foreach ( QString myId, m_accountIds )
    {
        QString myUsername;
        {
            Account::Configuration cfg;
            load( myId, cfg );
            myUsername = cfg.credentials[ "username" ].toString();
        }

        if ( myUsername.isEmpty() )
        {
            toDelete.insert( myId );
            continue;
        }

        foreach ( QString otherId, other->accountIds() )
        {
            bool typesMatch = false;
            foreach ( QString prefix, m_allowedPrefixes )
            {
                if ( otherId.startsWith( prefix ) && myId.startsWith( prefix ) )
                {
                    typesMatch = true;
                    break;
                }
            }
            if ( !typesMatch )
                continue;

            QString otherUsername;
            {
                Account::Configuration cfg;
                other->load( otherId, cfg );
                otherUsername = cfg.credentials[ "username" ].toString();
            }

            if ( myUsername == otherUsername )
            {
                toDelete.insert( myId );
                break;
            }
        }
    }

    foreach ( QString id, toDelete )
    {
        m_accountIds.removeAll( id );
    }
}


void
Tomahawk::Accounts::TelepathyConfigStorage::save( const QString& accountId, const Account::Configuration& cfg )
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "externalaccounts/" + accountId );
    s->setValue( "enabled", cfg.enabled );
    s->setValue( "acl", cfg.acl );
    s->endGroup();
    s->sync();

    if ( !m_accountIds.contains( accountId ) )
        m_accountIds.append( accountId );
}


void
Tomahawk::Accounts::TelepathyConfigStorage::load( const QString& accountId, Account::Configuration& cfg ) const
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "externalaccounts/" + accountId );
    cfg.enabled = s->value( "enabled", true ).toBool();
    cfg.acl =     s->value( "acl", QVariantMap() ).toMap();
    s->endGroup();


    Tp::AccountPtr account = m_tpam->accountForObjectPath( accountIdToTelepathyPath( accountId ) );

    if ( !account->normalizedName().isEmpty() )
        cfg.accountFriendlyName = account->normalizedName();
    else if ( !account->parameters()[ "account" ].isNull() )
        cfg.accountFriendlyName = account->parameters()[ "account" ].toString();

    if ( cfg.accountFriendlyName.isEmpty() ) //this should never happen
        cfg.accountFriendlyName = accountId;

    QStringList types;
    types << "SipType";
    cfg.types = types;

    if ( account->serviceName() == "google-talk" ||
         account->parameters()[ "port" ].isNull() )
        cfg.configuration[ "port" ] = "5222";
    else
        cfg.configuration[ "port" ] = account->parameters()[ "port" ].toString();

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

    cfg.configuration[ "read-only" ] = true;
}


void
Tomahawk::Accounts::TelepathyConfigStorage::remove( const QString& accountId )
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "externalaccounts/" + accountId );
    s->remove( "enabled" );
    s->remove( "acl" );
    s->endGroup();
    s->remove( "externalaccounts/" + accountId );
}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::ConfigStorage, Tomahawk::Accounts::TelepathyConfigStorage )
