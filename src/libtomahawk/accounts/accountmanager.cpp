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


AccountManager::AccountManager()
    : QObject()
{
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
        qDebug() << "Checking directory for plugins:" << pluginDir;
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

        qDebug() << "Trying to load plugin:" << fileName;
        loadPluginFactory( fileName );
    }
}


QString
AccountManager::factoryFromId( const QString& pluginId ) const
{
    return pluginId.split( "_" ).first();
}


void
AccountManager::loadPluginFactory( const QString& path )
{
    QPluginLoader loader( path );
    QObject* plugin = loader.instance();
    if ( !plugin )
    {
        qDebug() << "Error loading plugin:" << loader.errorString();
    }

    AccountFactory* accountfactory = qobject_cast<AccountFactory*>( plugin );
    if ( accountfactory )
    {
        qDebug() << "Loaded plugin factory:" << loader.fileName() << accountfactory->factoryId() << accountfactory->prettyName();
        m_accountFactories[ accountfactory->factoryId() ] = accountfactory;
    } else
    {
        qDebug() << "Loaded invalid plugin.." << loader.fileName();
    }
}


void
AccountManager::loadFromConfig()
{
    QStringList pluginIds = TomahawkSettings::instance()->accountPlugins();
    foreach( const QString& pluginId, pluginIds )
    {
        QString pluginFactory = factoryFromId( pluginId );
        if( m_accountFactories.contains( pluginFactory ) )
        {
            Account* a = loadPlugin( pluginId );
            addAccountPlugin( a );
        }
    }
}



Account*
AccountManager::loadPlugin( const QString& pluginId )
{
    QString factoryName = factoryFromId( pluginId );

    Q_ASSERT( m_accountFactories.contains( factoryName ) );

    Account* account = m_accountFactories[ factoryName ]->createAccount( pluginId );

    // caller responsible for calling pluginAdded() and hookupPlugin
    return account;
}

void
AccountManager::addAccountPlugin( Account* account )
{
    m_accounts << account;

    //FIXME:
    //emit pluginAdded( account );
}


};

};
