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

#include "PortFwdThread.h"

#include "portfwd/portfwd.h"
#include "utils/Logger.h"
#include "Source.h"

#include <QCoreApplication>
#include <QNetworkInterface>
#include <QStringList>
#include <QTime>
#include <QTimer>


PortFwdThread::PortFwdThread( unsigned int port )
    : QThread()
    , m_port( port )
{
}


PortFwdThread::~PortFwdThread()
{
}


void
PortFwdThread::run()
{
    m_worker = QPointer< PortFwdWorker >( new PortFwdWorker( m_port ) );
    Q_ASSERT( m_worker );
    connect( m_worker.data(), SIGNAL( externalAddressDetected( QHostAddress, unsigned int ) ), this, SIGNAL( externalAddressDetected( QHostAddress, unsigned int ) ) );
    QTimer::singleShot( 0, m_worker.data(), SLOT( work() ) );
    exec();

    if ( m_worker.data()->externalPort() )
    {
        qDebug() << "Unregistering port fwd";
        m_worker.data()->unregister();
    }

    if ( m_worker )
        delete m_worker.data();
}


QPointer< PortFwdWorker >
PortFwdThread::worker() const
{
    return m_worker;
}


PortFwdWorker::PortFwdWorker( unsigned int port )
    : QObject()
    , m_externalPort( 0 )
    , m_port( port )
{
}


PortFwdWorker::~PortFwdWorker()
{
    delete m_portfwd;
}


void
PortFwdWorker::unregister()
{
    m_portfwd->remove( m_externalPort );
}


void
PortFwdWorker::work()
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );
    m_portfwd = new Portfwd();

    foreach ( QHostAddress ha, QNetworkInterface::allAddresses() )
    {
        if( ha.toString() == "127.0.0.1" ) continue;
        if( ha.toString().contains( ":" ) ) continue; //ipv6

        m_portfwd->addBlockedDevice( ha.toString().toStdString() );
    }

    // try and pick an available port:
    if ( m_portfwd->init( 2000 ) )
    {
        int tryport = m_port;

        // last.fm office firewall policy hack
        // (corp. firewall allows outgoing connections to this port,
        //  so listen on this if you want lastfmers to connect to you)
        if ( qApp->arguments().contains( "--porthack" ) )
        {
            tryport = 3389;
            m_portfwd->remove( tryport );
        }

        for ( int r = 0; r < 3; ++r )
        {
            qDebug() << "Trying to setup portfwd on" << tryport;
            if ( m_portfwd->add( tryport, m_port ) )
            {
                QString pubip = QString( m_portfwd->external_ip().c_str() ).trimmed();
                m_externalAddress = QHostAddress( pubip );
                m_externalPort = tryport;
                tDebug() << "External servent address detected as" << pubip << ":" << m_externalPort;
                qDebug() << "Max upstream  " << m_portfwd->max_upstream_bps() << "bps";
                qDebug() << "Max downstream" << m_portfwd->max_downstream_bps() << "bps";
                break;
            }
            tryport = qAbs( 10000 + 50000 * (float)qrand() / RAND_MAX );
        }
    }
    else
        tDebug() << "No UPNP Gateway device found?";

    if ( !m_externalPort )
        tDebug() << "Could not setup fwd for port:" << m_port;

    emit externalAddressDetected( m_externalAddress, m_externalPort );
}

