/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "accountmanager.h"
#include "config.h"

#include <QtCore/QLibrary>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QCoreApplication>

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
    loadPluginFactories( findPluginFactories() );
}

    
AccountManager::~AccountManager()
{

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


QString
AccountManager::factoryFromId( const QString& accountId ) const
{
    return accountId.split( "_" ).first();
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
AccountManager::loadFromConfig()
{
    QStringList accountIds = TomahawkSettings::instance()->accounts();

    //FIXME: this is just for debugging
    if ( accountIds.isEmpty() )
    {
        Account* account = m_accountFactories[ "twitteraccount" ]->createAccount();
        addAccountPlugin( account );
        TomahawkSettings::instance()->addAccount( account->accountId() );
    }

    foreach( const QString& accountId, accountIds )
    {
        QString pluginFactory = factoryFromId( accountId );
        if( m_accountFactories.contains( pluginFactory ) )
        {
            Account* account = loadPlugin( accountId );
            addAccountPlugin( account );
        }
    }
}



Account*
AccountManager::loadPlugin( const QString& accountId )
{
    QString factoryName = factoryFromId( accountId );

    Q_ASSERT( m_accountFactories.contains( factoryName ) );

    Account* account = m_accountFactories[ factoryName ]->createAccount( accountId );

    // caller responsible for calling pluginAdded() and hookupPlugin
    return account;
}

void
AccountManager::addAccountPlugin( Account* account )
{
    tDebug() << Q_FUNC_INFO << "adding account plugin";
    m_accounts.append( account );

    foreach( AccountType type, account->types() )
        m_accountsByAccountType[ type ].append( account );

    emit accountAdded( account );
}


};

};
