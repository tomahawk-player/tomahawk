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

#ifndef TOMAHAWKUTILS_H
#define TOMAHAWKUTILS_H

#include "dllmacro.h"
#include <QObject>
#include <QThread>
#include <QNetworkProxy>
#include <QStringList>

#define RESPATH ":/data/"

class QColor;
class QDir;
class QDateTime;
class QString;
class QPixmap;
class QNetworkAccessManager;
class QNetworkProxy;

namespace TomahawkUtils
{
    class DLLEXPORT Sleep : public QThread
    {
    public:
        static void sleep( unsigned long secs )
        {
            QThread::sleep( secs );
        }
        static void msleep( unsigned long msecs )
        {
            QThread::msleep( msecs );
        }
        static void usleep( unsigned long usecs )
        {
            QThread::usleep( usecs );
        }
    };

    class DLLEXPORT NetworkProxyFactory : public QNetworkProxyFactory
    {
    public:
        NetworkProxyFactory() {}
        virtual ~NetworkProxyFactory() {}

        virtual QList< QNetworkProxy > queryProxy( const QNetworkProxyQuery & query = QNetworkProxyQuery() );
        static QList< QNetworkProxy > proxyForQuery( const QNetworkProxyQuery & query );

        void setNoProxyHosts( const QStringList &hosts );
        QStringList noProxyHosts() const { return m_noProxyHosts; }
        void setProxy( const QNetworkProxy &proxy );
        QNetworkProxy proxy() { return m_proxy; }

    private:
        QStringList m_noProxyHosts;
        QNetworkProxy m_proxy;
    };

    DLLEXPORT QDir appConfigDir();
    DLLEXPORT QDir appDataDir();
    DLLEXPORT QDir appLogDir();

    DLLEXPORT QString timeToString( int seconds );
    DLLEXPORT QString ageToString( const QDateTime& time );
    DLLEXPORT QString filesizeToString( unsigned int size );
    DLLEXPORT QString extensionToMimetype( const QString& extension );

    DLLEXPORT QColor alphaBlend( const QColor& colorFrom, const QColor& colorTo, float opacity );
    DLLEXPORT QPixmap createDragPixmap( int itemCount = 1 );

    DLLEXPORT NetworkProxyFactory* proxyFactory();
    DLLEXPORT QNetworkAccessManager* nam();

    DLLEXPORT void setProxyFactory( TomahawkUtils::NetworkProxyFactory* factory );
    DLLEXPORT void setNam( QNetworkAccessManager* nam );
}

#endif // TOMAHAWKUTILS_H
