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

#define RESPATH ":/data/"

class QDir;
class QDateTime;
class QString;
class QPixmap;
class QNetworkAccessManager;
class QNetworkProxy;

class JDnsShared;
class JDnsSharedRequest;

namespace TomahawkUtils
{
    //NOTE: The JDnsShared system is supposed to allow you to make multiple requests
    //at once, but either I'm a dumbass, or it's a broken paradigm, or both,
    //because there's no way that I can see to tell what result is for what query.
    //Be aware of this if ever we want to do parallel connections/lookups; turn it into
    //a non-static non-singleton normal utility class then.
    class DLLEXPORT DNSResolver : public QObject
    {
    Q_OBJECT

    public:
        explicit DNSResolver();
        ~DNSResolver() {}

        void resolve( QString& host, QString type );

    signals:
        void result( QString& result );

    public slots:
        void resultsReady();

    private:
        JDnsShared* m_dnsShared;
        JDnsSharedRequest* m_dnsSharedRequest;
    };

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

    DLLEXPORT QDir appConfigDir();
    DLLEXPORT QDir appDataDir();
    DLLEXPORT QDir appLogDir();

    DLLEXPORT QString timeToString( int seconds );
    DLLEXPORT QString ageToString( const QDateTime& time );
    DLLEXPORT QString filesizeToString( unsigned int size );
    DLLEXPORT QString extensionToMimetype( const QString& extension );

    DLLEXPORT QPixmap createDragPixmap( int itemCount = 1 );

    DLLEXPORT QNetworkAccessManager* nam();
    DLLEXPORT QNetworkProxy* proxy();

    DLLEXPORT void setNam( QNetworkAccessManager* nam );
    DLLEXPORT void setProxy( QNetworkProxy* proxy );

    DLLEXPORT DNSResolver* dnsResolver();
}

#endif // TOMAHAWKUTILS_H
