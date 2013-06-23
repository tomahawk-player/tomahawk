/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Dominik Schmidt <domme@tomahawk-player.org>
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

#include "PluginLoader.h"
#include "PluginLoader_p.h"

#include "config.h"

#include "utils/Logger.h"

#include <QDir>
#include <QCoreApplication>

#include "DllMacro.h"


namespace Tomahawk
{

namespace Utils
{


PluginLoader::PluginLoader( const QString& type )
    : d_ptr( new PluginLoaderPrivate( this ) )
{
    Q_D( PluginLoader );

    d->type = type;
}


PluginLoader::~PluginLoader()
{
    delete d_ptr;
}


const QList< QDir >
PluginLoader::pluginDirs()
{
    QList< QDir > pluginDirs;

    QDir appDir( QCoreApplication::instance()->applicationDirPath() );
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
    return pluginDirs;
}


const QStringList
PluginLoader::pluginFilenames( const QString& name ) const
{
    //TODO: ifdef!
    const QStringList extensions = QStringList()
            << "so"
            << "dll"
            << "dylib";


    QStringList fileNames;
    foreach( const QString& extension, extensions )
    {
        fileNames << QString("libtomahawk_%1_%2.%3")
                     .arg( d_ptr->type )
                     .arg( name )
                     .arg( extension );
    }

    return fileNames;
}


const QStringList
PluginLoader::pluginPaths( const QString& name ) const
{
    const QString type = d_ptr->type;

    QSet< QString > paths;
    foreach ( const QDir& pluginDir, pluginDirs() )
    {
        tDebug() << Q_FUNC_INFO << "Checking directory for" << type << "plugins:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( pluginFilenames( name ), QDir::Files ) )
        {
            //TODO: do we really need to check this?!
            if ( fileName.startsWith( QString( "libtomahawk_%1" ).arg( type ) ) )
            {
                const QString path = pluginDir.absoluteFilePath( fileName );
                paths << path;
            }
        }
    }

    tDebug() << Q_FUNC_INFO << type << "plugin file paths:" << paths;

    return paths.toList();
}


} // ns utils

} // ns Tomahawk
