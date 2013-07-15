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

#ifndef TOMAHAWK_UTILS_NETWORKPROXYFACTORY_H
#define TOMAHAWK_UTILS_NETWORKPROXYFACTORY_H

#include <QNetworkProxyFactory>
#include <QStringList>

#include "DllMacro.h"

namespace Tomahawk
{
namespace Utils
{
    class DLLEXPORT NetworkProxyFactory : public QNetworkProxyFactory
    {
    public:
        NetworkProxyFactory()
        : m_proxy( QNetworkProxy::NoProxy )
        , m_proxyChanged( false )
        {}

        NetworkProxyFactory( const NetworkProxyFactory &other );
        virtual ~NetworkProxyFactory() {}

        virtual QList< QNetworkProxy > queryProxy( const QNetworkProxyQuery & query = QNetworkProxyQuery() );

        virtual void setNoProxyHosts( const QStringList &hosts );
        virtual QStringList noProxyHosts() const { return m_noProxyHosts; }
        virtual void setProxy( const QNetworkProxy &proxy, bool useProxyDns );
        virtual QNetworkProxy proxy() { return m_proxy; }

        virtual NetworkProxyFactory& operator=( const NetworkProxyFactory &rhs );
        virtual bool operator==( const NetworkProxyFactory &other ) const;
        bool changed() const { return m_proxyChanged; }

    private:
        QStringList m_noProxyHosts;
        QNetworkProxy m_proxy;
        bool m_proxyChanged;
    };

    DLLEXPORT void setProxyFactory( NetworkProxyFactory* factory, bool noMutexLocker = false );
    DLLEXPORT NetworkProxyFactory* proxyFactory( bool makeClone = false, bool noMutexLocker = false );
}
}

#endif // TOMAHAWK_UTILS_NETWORKPROXYFACTORY_H
