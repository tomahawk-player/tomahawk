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
#include <sip/PeerInfo.h>
#include <utils/Logger.h>
#include <SourceList.h>

#include <QFile>
#include <QUuid>

HatchetSipPlugin::HatchetSipPlugin( Tomahawk::Accounts::Account *account )
    : SipPlugin( account )
    , m_sipState( Closed )
    , m_version( 0 )
    , m_publicKey( nullptr )
{
    tLog() << Q_FUNC_INFO;

    connect( m_account, SIGNAL( accessTokensFetched() ), this, SLOT( connectWebSocket() ) );
    connect( Servent::instance(), SIGNAL( dbSyncTriggered() ), this, SLOT( dbSyncTriggered() ));

    QFile pemFile( ":/hatchet-account/dreamcatcher.pem" );
    pemFile.open( QIODevice::ReadOnly );
    tLog() << Q_FUNC_INFO << "certs/dreamcatcher.pem: " << pemFile.readAll();
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
    return m_account->enabled() && m_account->isAuthenticated() && m_publicKey;
}


void
HatchetSipPlugin::sendSipInfo(const Tomahawk::peerinfo_ptr& receiver, const SipInfo& info)
{
    const QString dbid = receiver->data().toMap().value( "dbid" ).toString();
    tLog() << Q_FUNC_INFO << "Send local info to " << receiver->friendlyName() << "(" << dbid << ") we are" << info.nodeId() << "with offerkey " << info.key();

    QVariantMap sendMap;
    sendMap[ "command" ] = "authorize-peer";
    sendMap[ "dbid" ] = dbid;
    sendMap[ "offerkey" ] = info.key();


    if ( !sendBytes( sendMap ) )
        tLog() << Q_FUNC_INFO << "Failed sending message";
}

Tomahawk::Accounts::HatchetAccount*
HatchetSipPlugin::hatchetAccount() const
{
    return qobject_cast< Tomahawk::Accounts::HatchetAccount* >( m_account );
}


void
HatchetSipPlugin::connectPlugin()
{
    if ( !m_account->isAuthenticated() )
    {
        //FIXME: Prompt user for password?
        return;
    }

    hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Connecting );
    hatchetAccount()->fetchAccessTokens();
}


void
HatchetSipPlugin::disconnectPlugin()
{
    if ( m_webSocketThreadController && m_webSocketThreadController->isRunning() )
        emit disconnectWebSocket();
    else
        webSocketDisconnected();
}


///////////////////////////// Connection methods ////////////////////////////////////


void
HatchetSipPlugin::connectWebSocket()
{
    if ( m_webSocketThreadController )
        return;

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

    QVariantList tokensCreds = m_account->credentials()[ "accesstokens" ].toList();
    //FIXME: Don't blindly pick the first one that matches?
    QVariantMap connectVals;
    foreach ( QVariant credObj, tokensCreds )
    {
        QVariantMap creds = credObj.toMap();
        if ( creds.contains( "type" ) && creds[ "type" ].toString() == "dreamcatcher" )
        {
            connectVals = creds;
            m_userid = creds["userid"].toString();
            m_token = creds["token"].toString();
            break;
        }
    }

    QString url;
    if ( !connectVals.isEmpty() )
        url = connectVals[ "host" ].toString() + ':' + connectVals[ "port" ].toString();

    if ( url.isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "Unable to find a proper connection endpoint; bailing";
        disconnectPlugin();
        return;
    }
    else
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

    m_uuid = QUuid::createUuid().toString();
    QCA::SecureArray sa( m_uuid.toLatin1() );
    QCA::SecureArray result = m_publicKey->encrypt( sa, QCA::EME_PKCS1_OAEP );

    tLog() << Q_FUNC_INFO << "uuid:" << m_uuid << ", size of uuid:" << m_uuid.size() << ", size of sa:" << sa.size() << ", size of result:" << result.size();

    QVariantMap nonceVerMap;
    nonceVerMap[ "version" ] = VERSION;
    nonceVerMap[ "nonce" ] = QString( result.toByteArray().toBase64() );
    sendBytes( nonceVerMap );
}


void
HatchetSipPlugin::webSocketDisconnected()
{
    tLog() << Q_FUNC_INFO << "WebSocket disconnected";
    if ( m_webSocketThreadController )
    {
        m_webSocketThreadController->quit();
        m_webSocketThreadController->wait( 60000 );

        delete m_webSocketThreadController.data();
    }

    m_sipState = Closed;
    m_version = 0;

    hatchetAccount()->setConnectionState( Tomahawk::Accounts::Account::Disconnected );
    m_sipState = Closed;
}


bool
HatchetSipPlugin::sendBytes( const QVariantMap& jsonMap ) const
{
    tLog() << Q_FUNC_INFO;
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

    tLog() << Q_FUNC_INFO << "Sending bytes of size" << bytes.size();
    emit rawBytes( bytes );
    return true;
}


void
HatchetSipPlugin::messageReceived( const QByteArray &msg )
{
    tLog() << Q_FUNC_INFO << "WebSocket message: " << msg;

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
        tLog() << Q_FUNC_INFO << "In acquiring version state, expecting version/nonce information";
        if ( !retMap.contains( "version" ) || !retMap.contains( "nonce" ) )
        {
            tLog() << Q_FUNC_INFO << "Failed to acquire version or nonce information";
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

        if ( retMap[ "nonce" ].toString() != m_uuid )
        {
            tLog() << Q_FUNC_INFO << "Failed to validate nonce";
            disconnectPlugin();
            return;
        }

        m_version = ver;

        QVariantMap registerMap;
        registerMap[ "command" ] = "register";
        registerMap[ "userid" ] = m_userid;
        registerMap[ "host" ] = Servent::instance()->externalAddress();
        registerMap[ "port" ] = Servent::instance()->externalPort();
        registerMap[ "token" ] = m_token;
        registerMap[ "dbid" ] = Database::instance()->impl()->dbid();
        registerMap[ "alias" ] = QHostInfo::localHostName();

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
        tLog() << Q_FUNC_INFO << "Unable to convert and/or interepret command from server";
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
    const QString dbid = valMap[ "dbid" ].toString();
    const QString host = valMap[ "host" ].toString();
    unsigned int port = valMap[ "port" ].toUInt();

    tLog() << Q_FUNC_INFO << "username:" << username << "dbid" << dbid;

    QStringList keys( QStringList() << "command" << "username" << "host" << "port" << "dbid" );
    if ( !checkKeys( keys, valMap ) )
        return;

    Tomahawk::peerinfo_ptr peerInfo = Tomahawk::PeerInfo::get( this, dbid, Tomahawk::PeerInfo::AutoCreate );
    peerInfo->setContactId( username );
    peerInfo->setFriendlyName( username );
    QVariantMap data;
    data.insert( "dbid", QVariant::fromValue< QString >( dbid ) );
    peerInfo->setData( data );


    SipInfo sipInfo;
    sipInfo.setNodeId( dbid );
    if( !host.isEmpty() && port != 0 )
    {
        sipInfo.setHost( valMap[ "host" ].toString() );
        sipInfo.setPort( valMap[ "port" ].toUInt() );
        sipInfo.setVisible( true );
    }
    else
    {
        sipInfo.setVisible( false );
    }
    peerInfo->setSipInfo( sipInfo );

    peerInfo->setStatus( Tomahawk::PeerInfo::Online );
}


void
HatchetSipPlugin::peerAuthorization( const QVariantMap& valMap )
{
    tLog() << Q_FUNC_INFO << "dbid:" << valMap[ "dbid" ].toString() << "offerkey" << valMap[ "offerkey" ].toString();

    QStringList keys( QStringList() << "command" << "dbid" << "offerkey" );
    if ( !checkKeys( keys, valMap ) )
        return;


    Tomahawk::peerinfo_ptr peerInfo = Tomahawk::PeerInfo::get( this, valMap[ "dbid" ].toString() );
    if( peerInfo.isNull() )
    {
        tLog() << Q_FUNC_INFO << "Received a peer-authorization for a peer we don't know about";
        return;
    }

    SipInfo sipInfo = peerInfo->sipInfo();
    sipInfo.setKey( valMap[ "offerkey" ].toString() );
    peerInfo->setSipInfo( sipInfo );
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
    tLog() << Q_FUNC_INFO;
    DatabaseCommand_loadOps* cmd = new DatabaseCommand_loadOps( SourceList::instance()->getLocal(), valMap[ "lastrevision" ].toString() );
    connect( cmd, SIGNAL( done( QString, QString, QList< dbop_ptr > ) ), SLOT( oplogFetched( QString, QString, QList< dbop_ptr > ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
HatchetSipPlugin::oplogFetched( const QString& sinceguid, const QString& /* lastguid */, const QList< dbop_ptr > ops ) const
{
    tLog() << Q_FUNC_INFO;
    const uint_fast32_t byteMax = 1 << 25;
    int currBytes = 0;
    QVariantMap commandMap;
    commandMap[ "command" ] = "oplog";
    commandMap[ "startingrevision" ] = sinceguid;
    currBytes += 60; //baseline for quotes, keys, commas, colons, etc.
    QVariantList revisions;
    tLog() << Q_FUNC_INFO << "Found" << ops.size() << "ops";
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
    tLog() << Q_FUNC_INFO << "Sending" << revisions.size() << "revisions";
    commandMap[ "revisions" ] = revisions;

    if ( !sendBytes( commandMap ) )
    {
        tLog() << Q_FUNC_INFO << "Failed sending message, attempting to send a blank message to clear sync state";
        QVariantMap rescueMap;
        rescueMap[ "command" ] = "oplog";
        if ( !sendBytes( rescueMap ) )
        {
            tLog() << Q_FUNC_INFO << "Failed to send rescue map; state may be out-of-sync with server";
            //FIXME: Do we want to disconnect and reconnect at this point to try to get sending working and clear the server state?
        }
    }
}


