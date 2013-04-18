/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "Zeroconf.h"

#include "TomahawkSettings.h"
#include "utils/Logger.h"
#include "ZeroconfAccount.h"
#include "Source.h"
#include "sip/PeerInfo.h"
#include "network/ControlConnection.h"

#include <QtPlugin>
#include <QTimer>

using namespace Tomahawk;
using namespace Accounts;


ZeroconfPlugin::ZeroconfPlugin ( ZeroconfAccount* parent )
    : SipPlugin( parent )
    , m_zeroconf( 0 )
    , m_state( Account::Disconnected )
    , m_cachedNodes()
{
    qDebug() << Q_FUNC_INFO;
    m_advertisementTimer.setInterval( 60000 );
    m_advertisementTimer.setSingleShot( false );
    connect( &m_advertisementTimer, SIGNAL( timeout() ), this, SLOT( advertise() ) );
}


ZeroconfPlugin::~ZeroconfPlugin()
{
}


const QString
ZeroconfPlugin::name() const
{
    return QString( MYNAME );
}


const QString
ZeroconfPlugin::accountName() const
{
    return QString( MYNAME );
}


const QString
ZeroconfPlugin::serviceName() const
{
    return QString( MYNAME );
}


const QString
ZeroconfPlugin::friendlyName() const
{
    return QString( MYNAME );
}


Account::ConnectionState
ZeroconfPlugin::connectionState() const
{
    return m_state;
}


void
ZeroconfPlugin::connectPlugin()
{
    delete m_zeroconf;
    m_zeroconf = new TomahawkZeroconf( Servent::instance()->port(), this );
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( QString, int, QString, QString ) ),
                                    SLOT( lanHostFound( QString, int, QString, QString ) ) );

    advertise();
    m_state = Account::Connected;

    foreach( const QStringList& nodeSet, m_cachedNodes )
    {
        lanHostFound( nodeSet[0], nodeSet[1].toInt(), nodeSet[2], nodeSet[3]);
    }
    m_cachedNodes.clear();

    m_advertisementTimer.start();
}


void
ZeroconfPlugin::disconnectPlugin()
{
    m_advertisementTimer.stop();
    m_state = Account::Disconnected;

    delete m_zeroconf;
    m_zeroconf = 0;

    setAllPeersOffline();
}


#ifndef ENABLE_HEADLESS
QIcon
ZeroconfPlugin::icon() const
{
    return account()->icon();
}
#endif


void
ZeroconfPlugin::advertise()
{
    m_zeroconf->advertise();
}


void
ZeroconfPlugin::lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid )
{
    if ( sender() != m_zeroconf )
        return;

    qDebug() << "Found LAN host:" << host << port << nodeid;

    if ( m_state != Account::Connected )
    {
        qDebug() << "Not online, so not connecting.";
        QStringList nodeSet;
        nodeSet << host << QString::number( port ) << name << nodeid;
        m_cachedNodes.append( nodeSet );
        return;
    }

    SipInfo sipInfo;
    sipInfo.setHost( host );
    sipInfo.setPort( port );
    sipInfo.setNodeId( nodeid );
    sipInfo.setKey( "whitelist" );
    sipInfo.setVisible( true );

    Tomahawk::peerinfo_ptr peerInfo = Tomahawk::PeerInfo::get( this, host, Tomahawk::PeerInfo::AutoCreate );
    peerInfo->setSipInfo( sipInfo );
    peerInfo->setContactId( host );
    peerInfo->setFriendlyName( name );
    peerInfo->setType( PeerInfo::Local );
    peerInfo->setStatus( PeerInfo::Online );
}


