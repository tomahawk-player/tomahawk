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

#include "portfwdthread.h"

#include <QApplication>
#include <QStringList>
#include <QTime>
#include <QTimer>

#include "portfwd/portfwd.h"
#include "utils/logger.h"


PortFwdThread::PortFwdThread( unsigned int port )
    : QThread()
    , m_externalPort( 0 )
    , m_port( port )
{
    moveToThread( this );
    start();
}


PortFwdThread::~PortFwdThread()
{
    qDebug() << Q_FUNC_INFO << "waiting for event loop to finish...";
    quit();
    wait( 1000 );

    delete m_portfwd;
}


void
PortFwdThread::work()
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );
    m_portfwd = new Portfwd();

    // try and pick an available port:
    if( m_portfwd->init( 2000 ) )
    {
        int tryport = m_port;

        // last.fm office firewall policy hack
        // (corp. firewall allows outgoing connections to this port,
        //  so listen on this if you want lastfmers to connect to you)
        if( qApp->arguments().contains( "--porthack" ) )
        {
            tryport = 3389;
            m_portfwd->remove( tryport );
        }

        for( int r = 0; r < 3; ++r )
        {
            qDebug() << "Trying to setup portfwd on" << tryport;
            if( m_portfwd->add( tryport, m_port ) )
            {
                QString pubip = QString( m_portfwd->external_ip().c_str() );
                m_externalAddress = QHostAddress( pubip );
                m_externalPort = tryport;
                qDebug() << "External servent address detected as" << pubip << ":" << m_externalPort;
                qDebug() << "Max upstream  " << m_portfwd->max_upstream_bps() << "bps";
                qDebug() << "Max downstream" << m_portfwd->max_downstream_bps() << "bps";
                break;
            }
            tryport = qAbs( 10000 + 50000 * (float)qrand() / RAND_MAX );
        }
    }
    else
        qDebug() << "No UPNP Gateway device found?";

    if( !m_externalPort )
        qDebug() << "Could not setup fwd for port:" << m_port;

    emit externalAddressDetected( m_externalAddress, m_externalPort );
}


void
PortFwdThread::run()
{
    QTimer::singleShot( 0, this, SLOT( work() ) );
    exec();

    if ( m_externalPort )
    {
        qDebug() << "Unregistering port fwd";
        m_portfwd->remove( m_externalPort );
    }
}
