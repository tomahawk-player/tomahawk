/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TOMAHAWK_UTILS_NETWORKACCESSMANAGER_H
#define TOMAHAWK_UTILS_NETWORKACCESSMANAGER_H


#include <QNetworkProxy>

#include "DllMacro.h"

class QNetworkAccessManager;

namespace Tomahawk
{
namespace Utils
{
    DLLEXPORT QNetworkAccessManager* nam();
    DLLEXPORT void setNam( QNetworkAccessManager* nam, bool noMutexLocker = false );

    // Proxy settings
    DLLEXPORT bool proxyDns();
    DLLEXPORT void setProxyDns( bool proxyDns );

    DLLEXPORT QNetworkProxy::ProxyType proxyType();
    DLLEXPORT void setProxyType( QNetworkProxy::ProxyType proxyType );

    DLLEXPORT QString proxyHost();
    DLLEXPORT void setProxyHost( const QString& proxyHost );

    DLLEXPORT qulonglong proxyPort();
    DLLEXPORT void setProxyPort( qulonglong proxyPort );

    DLLEXPORT QString proxyUsername();
    DLLEXPORT void setProxyUsername( const QString& proxyUsername );

    DLLEXPORT QString proxyPassword();
    DLLEXPORT void setProxyPassword( const QString& proxyPassword );

    DLLEXPORT QString proxyNoProxyHosts();
    DLLEXPORT void setProxyNoProxyHosts( const QString& proxyNoProxyHosts );
}
}

#endif // TOMAHAWK_UTILS_NETWORKACCESSMANAGER_H
