/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "AccountManager.h"
#include "config.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "ResolverAccount.h"
#include "utils/Logger.h"
#include "sip/SipStatusMessage.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"

#include <QtCore/QLibrary>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QCoreApplication>
#include <QTimer>

namespace Tomahawk
{

namespace Accounts
{


AccountManager* AccountManager::s_instance = 0;


AccountManager*
AccountManager::instance()
{
    return s_instance;
}


AccountManager::AccountManager( QObject *parent )
    : QObject( parent )
{
    s_instance = this;

    QTimer::singleShot( 0, this, SLOT( init() ) );
}


AccountManager::~AccountManager()
{
    disconnectAll();
    qDeleteAll( m_accounts );
    qDeleteAll( m_accountFactories );
}


void
AccountManager::init()
{
    if ( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().isNull() )
    {
        //We need the info system worker to be alive so that we can move info plugins into its thread
        QTimer::singleShot( 0, this, SLOT( init() ) );
        return;
    }

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );

    loadPluginFactories( findPluginFactories() );

    // We include the resolver factory manually, not in a plugin
    ResolverAccountFactory* f = new ResolverAccountFactory();
    m_accountFactories[ f->factoryId() ] = f;
    registerAccountFactoryForFilesystem( f );

    emit ready();
}


QStringList
AccountManager::findPluginFactories()
{
    QStringList paths;
    QList< QDir > pluginDirs;

    QDir appDir( qApp->applicationDirPath() );
#ifdef Q_WS_MAC
    if ( appDir.dirName() == "MacOS" )
    {
        // Development convenience-hack
        appDir.cdUp();
        appDir.cdUp();
        appDir.cdUp();
    }
#endif

    QDir libDir( CMAKE_INSTALL_PREFIX "/lib" );

    QDir lib64Dir( appDir );
    lib64Dir.cdUp();
    lib64Dir.cd( "lib64" );

    pluginDirs << appDir << libDir << lib64Dir << QDir( qApp->applicationDirPath() );
    foreach ( const QDir& pluginDir, pluginDirs )
    {
        tDebug() << Q_FUNC_INFO << "Checking directory for plugins:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( QStringList() << "*tomahawk_account_*.so" << "*tomahawk_account_*.dylib" << "*tomahawk_account_*.dll", QDir::Files ) )
        {
            if ( fileName.startsWith( "libtomahawk_account" ) )
            {
                const QString path = pluginDir.absoluteFilePath( fileName );
                if ( !paths.contains( path ) )
                    paths << path;
            }
        }
    }

    return paths;
}


void
AccountManager::loadPluginFactories( const QStringList& paths )
{
    foreach ( QString fileName, paths )
    {
        if ( !QLibrary::isLibrary( fileName ) )
            continue;

        tDebug() << Q_FUNC_INFO << "Trying to load plugin:" << fileName;
        loadPluginFactory( fileName );
    }
}


bool
AccountManager::hasPluginWithFactory( const QString& factory ) const
{
    foreach ( Account* account, m_accounts )
    {
        if ( factoryFromId( account->accountId() ) == factory )
            return true;
    }
    return false;

}


QString
AccountManager::factoryFromId( const QString& accountId ) const
{
    return accountId.split( "_" ).first();
}


AccountFactory*
AccountManager::factoryForAccount( Account* account ) const
{
    const QString factoryId = factoryFromId( account->accountId() );
    return m_accountFactories.value( factoryId, 0 );
}


void
AccountManager::loadPluginFactory( const QString& path )
{
    QPluginLoader loader( path );
    QObject* plugin = loader.instance();
    if ( !plugin )
    {
        tDebug() << Q_FUNC_INFO << "Error loading plugin:" << loader.errorString();
    }

    AccountFactory* accountfactory = qobject_cast<AccountFactory*>( plugin );
    if ( accountfactory )
    {
        tDebug() << Q_FUNC_INFO << "Loaded plugin factory:" << loader.fileName() << accountfactory->factoryId() << accountfactory->prettyName();
        m_accountFactories[ accountfactory->factoryId() ] = accountfactory;
    } else
    {
        tDebug() << Q_FUNC_INFO << "Loaded invalid plugin.." << loader.fileName();
    }
}



void
AccountManager::enableAccount( Account* account )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( account->enabled() )
        return;

    account->authenticate();

    if ( account->preventEnabling() )
        return;

    account->setEnabled( true );
    m_enabledAccounts << account;

    account->sync();
}


void
AccountManager::disableAccount( Account* account )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !account->enabled() )
        return;

    account->deauthenticate();
    account->setEnabled( false );
    m_enabledAccounts.removeAll( account );

    account->sync();
}


void
AccountManager::connectAll()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    foreach ( Account* acc, m_accounts )
    {
        if ( acc->enabled() )
        {
            acc->authenticate();
            m_enabledAccounts << acc;
        }

    }
    m_connected = true;
}


void
AccountManager::disconnectAll()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    foreach ( Account* acc, m_enabledAccounts )
        acc->deauthenticate();

    m_enabledAccounts.clear();
    SourceList::instance()->removeAllRemote();
    m_connected = false;
}


void
AccountManager::toggleAccountsConnected()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( m_connected )
        disconnectAll();
    else
        connectAll();
}


void
AccountManager::loadFromConfig()
{
    QStringList accountIds = TomahawkSettings::instance()->accounts();
    qDebug() << "LOADING ALL ACCOUNTS" << accountIds;
    foreach ( const QString& accountId, accountIds )
    {
        QString pluginFactory = factoryFromId( accountId );
        if ( m_accountFactories.contains( pluginFactory ) )
        {
            Account* account = loadPlugin( accountId );
            addAccount( account );
        }
    }
}


void
AccountManager::initSIP()
{
    tDebug() << Q_FUNC_INFO;
    foreach ( Account* account, accounts() )
    {
        hookupAndEnable( account, true );
    }
}


Account*
AccountManager::loadPlugin( const QString& accountId )
{
    QString factoryName = factoryFromId( accountId );

    Q_ASSERT( m_accountFactories.contains( factoryName ) );

    Account* account = m_accountFactories[ factoryName ]->createAccount( accountId );
    hookupAccount( account );

    return account;
}


void
AccountManager::addAccount( Account* account )
{
    tDebug() << Q_FUNC_INFO << "adding account plugin";
    m_accounts.append( account );

    if ( account->types() & Accounts::SipType )
        m_accountsByAccountType[ Accounts::SipType ].append( account );
    if ( account->types() & Accounts::InfoType )
        m_accountsByAccountType[ Accounts::InfoType ].append( account );
    if ( account->types() & Accounts::ResolverType )
        m_accountsByAccountType[ Accounts::ResolverType ].append( account );
    if ( account->types() & Accounts::StatusPushType )
        m_accountsByAccountType[ Accounts::StatusPushType ].append( account );

    emit added( account );
}


void
AccountManager::removeAccount( Account* account )
{
    account->deauthenticate();

    // emit before moving from list so accountmodel can get indexOf
    emit removed( account );

    m_accounts.removeAll( account );
    m_enabledAccounts.removeAll( account );
    m_connectedAccounts.removeAll( account );
    foreach ( AccountType type, m_accountsByAccountType.keys() )
    {
        QList< Account* > accounts = m_accountsByAccountType.value( type );
        accounts.removeAll( account );
        m_accountsByAccountType[ type ] = accounts;
    }

    ResolverAccount* raccount = qobject_cast< ResolverAccount* >( account );
    if ( raccount )
        raccount->removeBundle();

    TomahawkSettings::instance()->removeAccount( account->accountId() );

    account->removeFromConfig();
    account->deleteLater();
}


QList< Account* >
AccountManager::accountsFromFactory( AccountFactory* factory ) const
{
    QList< Account* > accts;
    foreach ( Account* acct, m_accounts )
    {
        if ( factoryForAccount( acct ) == factory )
            accts << acct;
    }
    return accts;
}


Account*
AccountManager::accountFromPath( const QString& accountPath )
{
    foreach ( AccountFactory* factory, m_factoriesForFilesytem )
    {
        if ( factory->acceptsPath( accountPath ) )
        {
            return factory->createFromPath( accountPath );
        }
    }

    Q_ASSERT_X( false, "Shouldn't have had no account factory accepting a path.. at least ResolverAccount!", "" );
    return 0;
}


void
AccountManager::registerAccountFactoryForFilesystem( AccountFactory* factory )
{
    m_factoriesForFilesytem.prepend( factory );
}


void
AccountManager::addAccountFactory( AccountFactory* factory )
{
    m_accountFactories[ factory->factoryId() ] = factory;
}


Account*
AccountManager::zeroconfAccount() const
{
    foreach ( Account* account, accounts() )
    {
        if ( account->sipPlugin() && account->sipPlugin()->serviceName() == "zeroconf" )
            return account;
    }

    return 0;
}


void
AccountManager::hookupAccount( Account* account ) const
{
    connect( account, SIGNAL( error( int, QString ) ), SLOT( onError( int, QString ) ), Qt::UniqueConnection );
    connect( account, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), SLOT( onStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), Qt::UniqueConnection );
}


void
AccountManager::hookupAndEnable( Account* account, bool startup )
{
    Q_UNUSED( startup );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    hookupAccount( account );
    if ( account->enabled() )
    {
        account->authenticate();
        m_enabledAccounts << account;
    }
}


void
AccountManager::onError( int code, const QString& msg )
{
    Account* account = qobject_cast< Account* >( sender() );
    Q_ASSERT( account );


    qWarning() << "Failed to connect to SIP:" << account->accountFriendlyName() << code << msg;

    SipStatusMessage* statusMessage;
    if ( code == Account::AuthError )
    {
        statusMessage = new SipStatusMessage( SipStatusMessage::SipLoginFailure, account->accountFriendlyName() );
        JobStatusView::instance()->model()->addJob( statusMessage );
    }
    else
    {
        statusMessage = new SipStatusMessage(SipStatusMessage::SipConnectionFailure, account->accountFriendlyName(), msg );
        JobStatusView::instance()->model()->addJob( statusMessage );
        QTimer::singleShot( 10000, account, SLOT( authenticate() ) );
    }
}


void
AccountManager::onSettingsChanged()
{
    foreach ( Account* account, m_accounts )
    {
        if ( account->types() & Accounts::SipType && account->sipPlugin() )
            account->sipPlugin()->checkSettings();
    }
}


void
AccountManager::onStateChanged( Account::ConnectionState state )
{
    Account* account = qobject_cast< Account* >( sender() );
    Q_ASSERT( account );

    if ( account->connectionState() == Account::Disconnected )
    {
        m_connectedAccounts.removeAll( account );
        emit disconnected( account );
    }
    else if ( account->connectionState() == Account::Connected )
    {
        m_connectedAccounts << account;
        emit connected( account );
    }

    emit stateChanged( account, state );
}


};

};
