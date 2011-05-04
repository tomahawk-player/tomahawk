/*
   Copyright 2009 Last.fm Ltd.
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "NetworkAccessManager.h"
#include "InternetConnectionMonitor.h"
#include <lastfm/ws.h>
#include <lastfm/misc.h>
#include <QCoreApplication>
#include <QNetworkRequest>
#ifdef WIN32
#include "win/IeSettings.h"
#include "win/Pac.h"
#endif
#ifdef __APPLE__
#include "mac/ProxyDict.h"
#endif


static struct NetworkAccessManagerInit
{
    // We do this upfront because then our Firehose QTcpSocket will have a proxy
    // set by default. As well as any plain QNetworkAcessManager stuff, and the
    // scrobbler
    // In theory we should do this every request in case the configuration
    // changes but that is fairly unlikely use case, init? Maybe we should
    // anyway..

    NetworkAccessManagerInit()
    {
    #ifdef WIN32
        IeSettings s;
        // if it's autodetect, we determine the proxy everytime in proxy()
        // we don't really want to do a PAC lookup here, as it times out
        // at two seconds, so that hangs startup
        if (!s.fAutoDetect && s.lpszProxy)
        {
            QUrl url( QString::fromUtf16((const unsigned short*)s.lpszProxy) );
            QNetworkProxy proxy( QNetworkProxy::HttpProxy );
            proxy.setHostName( url.host() );
            proxy.setPort( url.port() );
            QNetworkProxy::setApplicationProxy( proxy );
        }
    #endif
    #ifdef __APPLE__
        ProxyDict dict;
        if (dict.isProxyEnabled())
        {
            QNetworkProxy proxy( QNetworkProxy::HttpProxy );
            proxy.setHostName( dict.host );
            proxy.setPort( dict.port );

            QNetworkProxy::setApplicationProxy( proxy );
        }
    #endif
    }
} init;


namespace lastfm
{
    LASTFM_DLLEXPORT QByteArray UserAgent;
}


lastfm::NetworkAccessManager::NetworkAccessManager( QObject* parent )
               : QNetworkAccessManager( parent )
            #ifdef WIN32
               , m_pac( 0 )
               , m_monitor( 0 )
            #endif
{
    // can't be done in above init, as applicationName() won't be set
    if (lastfm::UserAgent.isEmpty())
    {
        QByteArray name = QCoreApplication::applicationName().toUtf8();
        QByteArray version = QCoreApplication::applicationVersion().toUtf8();
        if (version.size()) version.prepend( ' ' );
        lastfm::UserAgent = name + version + " (" + lastfm::platform() + ")";
    }
}


lastfm::NetworkAccessManager::~NetworkAccessManager()
{
#ifdef WIN32
    delete m_pac;
#endif
}


QNetworkProxy
lastfm::NetworkAccessManager::proxy( const QNetworkRequest& request )
{
    Q_UNUSED( request );

#ifdef WIN32
    IeSettings s;
    if (s.fAutoDetect)
    {
        if (!m_pac) {
            m_pac = new Pac;
            if ( !m_monitor )
            {
                m_monitor = new InternetConnectionMonitor( this );
                connect( m_monitor, SIGNAL( connectivityChanged( bool ) ), SLOT( onConnectivityChanged( bool ) ) );
            }
        }
        return m_pac->resolve( request, s.lpszAutoConfigUrl );
    }
#endif

    return QNetworkProxy::applicationProxy();
}


QNetworkReply*
lastfm::NetworkAccessManager::createRequest( Operation op, const QNetworkRequest& request_, QIODevice* outgoingData )
{
    QNetworkRequest request = request_;

    request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache );
    request.setRawHeader( "User-Agent", lastfm::UserAgent );

#ifdef WIN32
    // PAC proxies can vary by domain, so we have to check everytime :(
    QNetworkProxy proxy = this->proxy( request );
    if (proxy.type() != QNetworkProxy::NoProxy)
        QNetworkAccessManager::setProxy( proxy );
#endif

    return QNetworkAccessManager::createRequest( op, request, outgoingData );
}


void
lastfm::NetworkAccessManager::onConnectivityChanged( bool up )
{
    Q_UNUSED( up );

#ifdef WIN32
    if (up && m_pac) m_pac->resetFailedState();
#endif
}
