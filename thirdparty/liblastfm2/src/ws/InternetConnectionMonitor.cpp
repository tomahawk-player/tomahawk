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

#include "InternetConnectionMonitor.h"
#include "linux/LNetworkConnectionMonitor.h"
#include "mac/MNetworkConnectionMonitor.h"
#include "win/WNetworkConnectionMonitor.h"
#include "NetworkConnectionMonitor.h"
#include "ws.h"

lastfm::InternetConnectionMonitor::InternetConnectionMonitor( QObject *parent )
                                 : QObject( parent )
                                 , m_up( true )
{
    m_networkMonitor = createNetworkConnectionMonitor();

    if ( m_networkMonitor )
    {
        connect( m_networkMonitor, SIGNAL( networkUp() ), this, SLOT( onNetworkUp() ) );
        connect( m_networkMonitor, SIGNAL( networkDown() ), this, SLOT( onNetworkDown() ) );
    }

    connect( lastfm::nam(), SIGNAL( finished( QNetworkReply* ) ), this, SLOT( onFinished( QNetworkReply* ) ) );
}

void
lastfm::InternetConnectionMonitor::onFinished( QNetworkReply* reply )
{
    switch( reply->error() )
    {
        case QNetworkReply::NoError:
            if ( !m_up )
            {
                m_up = true;
                emit up();
                emit connectivityChanged( m_up );
            }
            break;
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyNotFoundError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::ProxyAuthenticationRequiredError:
            if ( m_up )
            {
                m_up = false;
                emit down();
                emit connectivityChanged( m_up );
            }
            break;
        default:
            break;
    }
}

void
lastfm::InternetConnectionMonitor::onNetworkUp()
{
#ifdef Q_OS_MAC
    // We don't need to check on mac as the
    // check is done as part of the reach api
    m_up = true;
    emit up();
    emit connectivityChanged( m_up );
#else
    qDebug() << "Network seems to be up again. Let's try if there's internet connection!";
    lastfm::nam()->head( QNetworkRequest( QUrl( tr( "http://www.last.fm/" ) ) ) );
#endif
}

void
lastfm::InternetConnectionMonitor::onNetworkDown()
{
    qDebug() << "Internet is down :( boo!!";
    m_up = false;
    emit down();
    emit connectivityChanged( m_up );
}

NetworkConnectionMonitor*
lastfm::InternetConnectionMonitor::createNetworkConnectionMonitor()
{
    NetworkConnectionMonitor* ncm = 0;

#ifdef Q_WS_X11
    ncm = new LNetworkConnectionMonitor( this );
#elif defined(Q_WS_WIN)
    ncm = new WNetworkConnectionMonitor( this );
#elif defined(Q_WS_MAC)
    ncm = new MNetworkConnectionMonitor( this );
#endif

    return ncm;
}
