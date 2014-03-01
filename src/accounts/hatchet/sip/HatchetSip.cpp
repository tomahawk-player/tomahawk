/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "HatchetSip.h"

#include "account/HatchetAccount.h"
#include "WebSocketThreadController.h"
//#include "WebSocket.h"

#include <database/Database.h>
#include <database/DatabaseImpl.h>
#include <database/DatabaseCommand_LoadOps.h>
#include <network/ControlConnection.h>
#include <network/Servent.h>
#include <sip/SipInfo.h>
#include <sip/PeerInfo.h>
#include <utils/Logger.h>
#include <SourceList.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QFile>
#include <QHostInfo>
#include <QUuid>

HatchetSipPlugin::HatchetSipPlugin( Tomahawk::Accounts::Account *account )
    : SipPlugin( account )
    , m_sipState( Closed )
    , m_version( 0 )
    , m_publicKey( nullptr )
    , m_reconnectTimer( this )
{
    tLog() << Q_FUNC_INFO;

    connect( m_account, SIGNAL( accessTokenFetched() ), this, SLOT( connectWebSocket() ) );
    connect( Servent::instance(), SIGNAL( dbSyncTriggered() ), this, SLOT( dbSyncTriggered() ));

    /*
    QFile pemFile( ":/hatchet-account/dreamcatcher.pem" );
    pemFile.open( QIODevice::ReadOnly );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "certs/dreamcatcher.pem: " << pemFile.readAll();
    pemFile.close();
    pemFile.open( QIODevice::ReadOnly );
    QCA::ConvertResult conversionResult;
    QCA::PublicKey publicKey = QCA::PublicKey::fromPEM(pemFile.readAll(), &conversionResult);
    if ( QCA::ConvertGood != conversionResult )
    {
        tLog() << Q_FUNC_INFO << "INVALID PUBKEY READ";
        return;
    }
    m_publicKey = new QCA::PublicKey( publicKey );
    */

    m_reconnectTimer.setInterval( 0 );
    m_reconnectTimer.setSingleShot( true );
    connect( &m_reconnectTimer, SIGNAL( timeout() ), SLOT( connectPlugin() ) );
}


HatchetSipPlugin::~HatchetSipPlugin()
{
    if ( m_webSocketThreadController )
    {
        m_webSocketThreadController->quit();
        m_webSocketThreadController->wait( 60000 );

        delete m_webSocketThreadController.data();
    }

    m_sipState = Closed;

    hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Disconnected );
}


bool
HatchetSipPlugin::isValid() const
{
    //return m_account->enabled() && m_account->isAuthenticated() && m_publicKey;
    return m_account->enabled() && m_account->isAuthenticated();
}


Tomahawk::Accounts::HatchetAccount*
HatchetSipPlugin::hatchetAccount() const
{
    return qobject_cast< Tomahawk::Accounts::HatchetAccount* >( m_account );
}


void
HatchetSipPlugin::connectPlugin()
{
    tLog() << Q_FUNC_INFO;
    if ( !m_account->isAuthenticated() )
    {
        tLog() << Q_FUNC_INFO << "Account not authenticated, not continuing";
        //FIXME: Prompt user for password?
        return;
    }

    hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Connecting );
    hatchetAccount()->fetchAccessToken( "dreamcatcher" );
}


void
HatchetSipPlugin::disconnectPlugin()
{
    tLog() << Q_FUNC_INFO;
    if ( m_webSocketThreadController && m_webSocketThreadController->isRunning() )
        emit disconnectWebSocket();
    else
        webSocketDisconnected();
}


///////////////////////////// Connection methods ////////////////////////////////////


void
HatchetSipPlugin::connectWebSocket()
{
    tLog() << Q_FUNC_INFO;
    if ( m_webSocketThreadController )
    {
        tLog() << Q_FUNC_INFO << "Already have a thread running, bailing";
        return;
    }

    m_webSocketThreadController = QPointer< WebSocketThreadController >( new WebSocketThreadController( this ) );

    if ( !m_webSocketThreadController )
    {
        tLog() << Q_FUNC_INFO << "Could not create a new thread, bailing";
        disconnectPlugin();
        return;
    }

    if ( !isValid() )
    {
      tLog() << Q_FUNC_INFO << "Invalid state, not continuing with connection";
      return;
    }

    m_token = m_account->credentials()[ "dreamcatcher_access_token" ].toString();


    if ( m_token.isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "Unable to find an access token"; 
        disconnectPlugin();
        return;
    }

    QString url( "wss://dreamcatcher.hatchet.is:443" );
    tLog() << Q_FUNC_INFO << "Connecting to Dreamcatcher endpoint at: " << url;

    m_webSocketThreadController->setUrl( url );
    m_webSocketThreadController->start();
}


void
HatchetSipPlugin::webSocketConnected()
{
    tLog() << Q_FUNC_INFO << "WebSocket connected";

    if ( m_token.isEmpty() || !m_account->credentials().contains( "username" ) )
    {
        tLog() << Q_FUNC_INFO << "access token or username is empty, aborting";
        disconnectPlugin();
        return;
    }

    hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Connected );
    m_sipState = AcquiringVersion;

    /*
    m_uuid = QUuid::createUuid().toString();
    QCA::SecureArray sa( m_uuid.toLatin1() );
    QCA::SecureArray result = m_publicKey->encrypt( sa, QCA::EME_PKCS1_OAEP );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "uuid:" << m_uuid << ", size of uuid:" << m_uuid.size() << ", size of sa:" << sa.size() << ", size of result:" << result.size();
    */

    QVariantMap nonceVerMap;
    nonceVerMap[ "version" ] = VERSION;
    //nonceVerMap[ "nonce" ] = QString( result.toByteArray().toBase64() );
    sendBytes( nonceVerMap );
}


void
HatchetSipPlugin::webSocketDisconnected()
{
    tLog() << Q_FUNC_INFO << "WebSocket disconnected";

    m_reconnectTimer.stop();

    if ( m_webSocketThreadController )
    {
        m_webSocketThreadController->quit();
        m_webSocketThreadController->wait( 60000 );

        delete m_webSocketThreadController.data();
    }

    m_sipState = Closed;
    m_version = 0;

    hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Disconnected );

    if ( hatchetAccount()->enabled() )
    {
        // Work on the assumption that we were disconnected because Dreamcatcher shut down
        // Reconnect after a time; use reasonable backoff + random
        int interval = m_reconnectTimer.interval() <= 25000 ? m_reconnectTimer.interval() + 5000 : 30000;
        interval += QCA::Random::randomInt() % 30;
        m_reconnectTimer.setInterval( interval );
        m_reconnectTimer.start();
    }
}


bool
HatchetSipPlugin::sendBytes( const QVariantMap& jsonMap ) const
{
    if ( m_sipState == Closed )
    {
        tLog() << Q_FUNC_INFO << "was told to send bytes on a closed connection, not gonna do it";
        return false;
    }

    QJson::Serializer serializer;
    QByteArray bytes = serializer.serialize( jsonMap );
    if ( bytes.isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "could not serialize register structure to JSON";
        return false;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Sending bytes of size" << bytes.size();
    emit rawBytes( bytes );
    return true;
}


void
HatchetSipPlugin::messageReceived( const QByteArray &msg )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "WebSocket message: " << msg;

    QJson::Parser parser;
    bool ok;
    QVariant jsonVariant = parser.parse( msg, &ok );
    if ( !jsonVariant.isValid() )
    {
        tLog() << Q_FUNC_INFO << "Failed to parse message back from server";
        return;
    }

    QVariantMap retMap = jsonVariant.toMap();

    if ( m_sipState == AcquiringVersion )
    {
        tLog() << Q_FUNC_INFO << "In acquiring version state, expecting versioninformation";
        if ( !retMap.contains( "version" ) )
        {
            tLog() << Q_FUNC_INFO << "Failed to acquire version information";
            disconnectPlugin();
            return;
        }
        bool ok = false;
        int ver = retMap[ "version" ].toInt( &ok );
        if ( ver == 0 || !ok )
        {
            tLog() << Q_FUNC_INFO << "Failed to acquire version information";
            disconnectPlugin();
            return;
        }

        /*
        if ( retMap[ "nonce" ].toString() != m_uuid )
        {
            tLog() << Q_FUNC_INFO << "Failed to validate nonce";
            disconnectPlugin();
            return;
        }
        */

        m_version = ver;

        QVariantMap registerMap;
        registerMap[ "command" ] = "register";
        registerMap[ "token" ] = m_token;
        registerMap[ "dbid" ] = Tomahawk::Database::instance()->impl()->dbid();
        registerMap[ "alias" ] = QHostInfo::localHostName();

        QList< SipInfo > sipinfos = Servent::instance()->getLocalSipInfos( "default", "default" );
        QVariantList hostinfos;
        foreach ( SipInfo sipinfo, sipinfos )
        {
            QVariantMap hostinfo;
            hostinfo[ "host" ] = sipinfo.host();
            hostinfo[ "port" ] = sipinfo.port();
            hostinfos << hostinfo;
        }

        registerMap[ "hostinfo" ] = hostinfos;

        if ( !sendBytes( registerMap ) )
        {
            tLog() << Q_FUNC_INFO << "Failed sending message";
            disconnectPlugin();
            return;
        }

        m_sipState = Registering;
    }
    else if ( m_sipState == Registering )
    {
        tLog() << Q_FUNC_INFO << "In registering state, checking status of registration";
        if ( retMap.contains( "status" ) &&
                retMap[ "status" ].toString() == "success" )
        {
            tLog() << Q_FUNC_INFO << "Registered successfully";
            m_sipState = Connected;
            hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Connected );
            m_reconnectTimer.setInterval( 0 );
            QTimer::singleShot(0, this, SLOT( dbSyncTriggered() ) );
            return;
        }
        else
        {
            tLog() << Q_FUNC_INFO << "Failed to register successfully";
            //m_ws.data()->stop();
            return;
        }
    }
    else if ( m_sipState != Connected )
    {
        // ...erm?
        tLog() << Q_FUNC_INFO << "Got a message from a non connected socket?";
        return;
    }
    else if ( !retMap.contains( "command" ) ||
                !retMap[ "command" ].canConvert< QString >() )
    {
        tDebug() << Q_FUNC_INFO << "Unable to convert and/or interepret command from server";
        return;
    }

    QString command = retMap[ "command" ].toString();

    if ( command == "new-peer" )
        newPeer( retMap );
    else if ( command == "peer-authorization" )
        peerAuthorization( retMap );
    else if ( command == "synclastseen" )
        sendOplog( retMap );
}


bool
HatchetSipPlugin::checkKeys( QStringList keys, const QVariantMap& map ) const
{
    foreach ( QString key, keys )
    {
        if ( !map.contains( key ) )
        {
            tLog() << Q_FUNC_INFO << "Did not find the value" << key << "in the new-peer structure";
            return false;
        }
    }
    return true;
}


///////////////////////////// Peer handling methods ////////////////////////////////////


void
HatchetSipPlugin::newPeer( const QVariantMap& valMap )
{
    const QString username = valMap[ "username" ].toString();
    const QVariantList hostinfo = valMap[ "hostinfo" ].toList();
    const QString dbid = valMap[ "dbid" ].toString();

    tDebug() << Q_FUNC_INFO << "username:" << username << "dbid" << dbid;

    QStringList keys( QStringList() << "command" << "username" << "hostinfo" << "dbid" );
    if ( !checkKeys( keys, valMap ) )
        return;

    Tomahawk::peerinfo_ptr peerInfo = Tomahawk::PeerInfo::get( this, dbid, Tomahawk::PeerInfo::AutoCreate );
    peerInfo->setContactId( username );
    peerInfo->setFriendlyName( username );
    QVariantMap data;
    data.insert( "dbid", dbid );
    peerInfo->setData( data );

    QList< SipInfo > sipInfos;

    foreach ( const QVariant listItem, hostinfo )
    {
        if ( !listItem.canConvert< QVariantMap >() )
            continue;

        QVariantMap hostpair = listItem.toMap();

        if ( !hostpair.contains( "host" ) || !hostpair.contains( "port" ) )
            continue;

        const QString host = hostpair[ "host" ].toString();
        unsigned int port = hostpair[ "port" ].toUInt();

        if ( host.isEmpty() || port == 0 )
            continue;

        SipInfo sipInfo;
        sipInfo.setNodeId( dbid );
        sipInfo.setHost( host );
        sipInfo.setPort( port );
        sipInfo.setVisible( true );
        sipInfos << sipInfo;
    }

    m_sipInfoHash[ dbid ] = sipInfos;

    peerInfo->setStatus( Tomahawk::PeerInfo::Online );
}


void
HatchetSipPlugin::sendSipInfos(const Tomahawk::peerinfo_ptr& receiver, const QList< SipInfo >& infos)
{
    if ( infos.size() == 0 )
    {
        tLog() << Q_FUNC_INFO << "Got no sipinfo data (list size 0)";
        return;
    }

    const QString dbid = receiver->data().toMap().value( "dbid" ).toString();
    tDebug() << Q_FUNC_INFO << "Send local info to " << receiver->friendlyName() << "(" << dbid << ") we are" << infos[ 0 ].nodeId() << "with offerkey " << infos[ 0 ].key();

    QVariantMap sendMap;
    sendMap[ "command" ] = "authorize-peer";
    sendMap[ "dbid" ] = dbid;
    sendMap[ "offerkey" ] = infos[ 0 ].key();

    if ( !sendBytes( sendMap ) )
        tLog() << Q_FUNC_INFO << "Failed sending message";
}


void
HatchetSipPlugin::peerAuthorization( const QVariantMap& valMap )
{
    tDebug() << Q_FUNC_INFO << "dbid:" << valMap[ "dbid" ].toString() << "offerkey" << valMap[ "offerkey" ].toString();

    QStringList keys( QStringList() << "command" << "dbid" << "offerkey" );
    if ( !checkKeys( keys, valMap ) )
        return;

    QString dbid = valMap[ "dbid" ].toString();

    Tomahawk::peerinfo_ptr peerInfo = Tomahawk::PeerInfo::get( this, dbid );
    if( peerInfo.isNull() )
    {
        tLog() << Q_FUNC_INFO << "Received a peer-authorization for a peer we don't know about";
        return;
    }

    QList< SipInfo > sipInfos = m_sipInfoHash[ dbid ];
    for (int i = 0; i < sipInfos.size(); i++)
        sipInfos[i].setKey( valMap[ "offerkey" ].toString() );
    peerInfo->setSipInfos( sipInfos );
    m_sipInfoHash.remove( dbid );
}


///////////////////////////// Syncing methods ////////////////////////////////////

void
HatchetSipPlugin::dbSyncTriggered()
{
    if ( m_sipState != Connected )
        return;

    if ( !SourceList::instance() || SourceList::instance()->getLocal().isNull() )
        return;

    QVariantMap sourceMap;
    sourceMap[ "command" ] = "synctrigger";
    const Tomahawk::source_ptr src = SourceList::instance()->getLocal();
    sourceMap[ "name" ] = src->friendlyName();
    sourceMap[ "alias" ] = QHostInfo::localHostName();
    sourceMap[ "friendlyname" ] = src->dbFriendlyName();

    if ( !sendBytes( sourceMap ) )
    {
        tLog() << Q_FUNC_INFO << "Failed sending message";
        return;
    }
}


void
HatchetSipPlugin::sendOplog( const QVariantMap& valMap ) const
{
    tDebug() << Q_FUNC_INFO;
    Tomahawk::DatabaseCommand_loadOps* cmd = new Tomahawk::DatabaseCommand_loadOps( SourceList::instance()->getLocal(), valMap[ "lastrevision" ].toString() );
    connect( cmd, SIGNAL( done( QString, QString, QList< dbop_ptr > ) ), SLOT( oplogFetched( QString, QString, QList< dbop_ptr > ) ) );
    Tomahawk::Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
HatchetSipPlugin::oplogFetched( const QString& sinceguid, const QString& /* lastguid */, const QList< dbop_ptr > ops )
{
    const uint_fast32_t byteMax = 1 << 25;
    int currBytes = 0;
    QVariantMap commandMap;
    commandMap[ "command" ] = "oplog";
    commandMap[ "startingrevision" ] = sinceguid;
    currBytes += 60; //baseline for quotes, keys, commas, colons, etc.
    QVariantList revisions;
    tDebug() << Q_FUNC_INFO << "Found" << ops.size() << "ops";
    foreach( const dbop_ptr op, ops )
    {
        currBytes += 80; //baseline for quotes, keys, commas, colons, etc.
        QVariantMap revMap;
        revMap[ "revision" ] = op->guid;
        currBytes += op->guid.length();
        revMap[ "singleton" ] = op->singleton;
        currBytes += 5; //true or false
        revMap[ "command" ] = op->command;
        currBytes += op->command.length();
        currBytes += 5; //true or false
        if ( op->compressed )
        {
            revMap[ "compressed" ] = true;
            QByteArray b64 = op->payload.toBase64();
            revMap[ "payload" ] = op->payload.toBase64();
            currBytes += b64.length();
        }
        else
        {
            revMap[ "compressed" ] = false;
            revMap[ "payload" ] = op->payload;
            currBytes += op->payload.length();
        }
        if ( currBytes >= (int)(byteMax - 1000000) ) // tack on an extra 1M for safety as it seems qjson puts in spaces
            break;
        else
          revisions << revMap;
    }
    tDebug() << Q_FUNC_INFO << "Sending" << revisions.size() << "revisions";
    commandMap[ "revisions" ] = revisions;

    if ( !sendBytes( commandMap ) )
    {
        tLog() << Q_FUNC_INFO << "Failed sending message, attempting to send a blank message to clear sync state";
        QVariantMap rescueMap;
        rescueMap[ "command" ] = "oplog";
        if ( !sendBytes( rescueMap ) )
        {
            tLog() << Q_FUNC_INFO << "Failed to send rescue map; state may be out-of-sync with server; reconnecting";
            disconnectPlugin();
        }
    }
}


