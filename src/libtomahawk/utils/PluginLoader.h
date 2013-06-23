/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#ifndef LIBTOMAHAWK_UTILS_PLUGINLOADER_H
#define LIBTOMAHAWK_UTILS_PLUGINLOADER_H

#include <QStringList>

#include "DllMacro.h"

class QDir;
class PluginLoaderPrivate;

namespace Tomahawk
{

namespace Utils
{

class DLLEXPORT PluginLoader
{
public:
    PluginLoader( const QString& type );
    virtual ~PluginLoader();

    const QHash< QString, QObject* > loadPlugins() const;

private:
    const QStringList pluginFilenames( const QString& name = "*" ) const;
    const QStringList pluginPaths( const QString& name = "*" ) const;

    static const QList< QDir > pluginDirs();

    Q_DECLARE_PRIVATE( PluginLoader );
    PluginLoaderPrivate* d_ptr;
};

}

}

#endif // LIBTOMAHAWK_UTILS_PLUGINLOADER_H
