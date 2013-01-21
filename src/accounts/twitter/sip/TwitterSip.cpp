/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TwitterSip.h"

#include "utils/TomahawkUtils.h"
#include "TomahawkSettings.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "network/Servent.h"
#include "Source.h"

#include "utils/Logger.h"
#include "accounts/twitter/TomahawkOAuthTwitter.h"
#include "accounts/twitter/TwitterAccount.h"

#include <QTweetLib/qtweetaccountverifycredentials.h>
#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetstatus.h>
#include <QTweetLib/qtweetusershow.h>

#include <QtPlugin>
#include <QDateTime>
#include <QRegExp>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>

static QString s_gotTomahawkRegex = QString( "^(@[a-zA-Z0-9]+ )?(Got Tomahawk\\?) (\\{[a-fA-F0-9\\-]+\\}) (.*)$" );

TwitterSipPlugin::TwitterSipPlugin( Tomahawk::Accounts::Account* account )
    : SipPlugin( account )
    , m_checkTimer( this )
    , m_connectTimer( this )
    , m_dmPollTimer( this )
    , m_cachedFriendsSinceId( 0 )
    , m_cachedMentionsSinceId( 0 )
    , m_cachedDirectMessagesSinceId( 0 )
    , m_cachedPeers()
    , m_keyCache()
    , m_state( Tomahawk::Accounts::Account::Disconnected )
{
    qDebug() << Q_FUNC_INFO;

    connect( account, SIGNAL( nowAuthenticated( const QPointer< TomahawkOAuthTwitter > &, const QTweetUser & ) ), SLOT( accountAuthenticated( const QPointer< TomahawkOAuthTwitter > &, const QTweetUser & ) ) );

    m_configuration = account->configuration();
    qDebug() << "SIP configuration:" << m_configuration << m_configuration[ "cachedpeers" ];
    if ( Database::instance()->impl()->dbid() != m_account->configuration()[ "saveddbid" ].toString() )
    {
        m_configuration[ "cachedpeers" ] = QVariantHash();
        m_configuration[ "saveddbid" ] = Database::instance()->impl()->dbid();
        syncConfig();
    }

    m_checkTimer.setInterval( 180000 );
    m_checkTimer.setSingleShot( false );
    connect( &m_checkTimer, SIGNAL( timeout() ), SLOT( checkTimerFired() ) );

    m_dmPollTimer.setInterval( 60000 );
    m_dmPollTimer.setSingleShot( false );
    connect( &m_dmPollTimer, SIGNAL( timeout() ), SLOT( pollDirectMessages() ) );

    m_connectTimer.setInterval( 180000 );
    m_connectTimer.setSingleShot( false );
    connect( &m_connectTimer, SIGNAL( timeout() ), SLOT( connectTimerFired() ) );
}


bool
TwitterSipPlugin::isValid() const
{
    return m_account->enabled() && m_account->isAuthenticated() && !m_cachedTwitterAuth.isNull();
}


Tomahawk::Accounts::Account::ConnectionState
TwitterSipPlugin::connectionState() const
{
    return m_state;
}

QString
TwitterSipPlugin::inviteString() const
{
    return tr( "Enter Twitter username" );
}


void
TwitterSipPlugin::checkSettings()
{
    configurationChanged();
}


void
TwitterSipPlugin::connectPlugin()
{
    tDebug() << Q_FUNC_INFO;
    if ( !m_account->enabled() )
    {
        tDebug() << Q_FUNC_INFO << "account isn't enabled";
        return;
    }

    m_cachedPeers = m_configuration[ "cachedpeers" ].toHash();
    QStringList peerList = m_cachedPeers.keys();
    qStableSort( peerList.begin(), peerList.end() );

    if ( !m_account->isAuthenticated() )
    {
        tDebug() << Q_FUNC_INFO << "account isn't authenticated, attempting";
        m_account->authenticate();
    }

    m_state = Tomahawk::Accounts::Account::Connecting;
    emit stateChanged( m_state );
}


void
TwitterSipPlugin::disconnectPlugin()
{
    tDebug() << Q_FUNC_INFO;
    m_checkTimer.stop();
    m_connectTimer.stop();
    m_dmPollTimer.stop();
    if( !m_friendsTimeline.isNull() )
        delete m_friendsTimeline.data();
    if( !m_mentions.isNull() )
        delete m_mentions.data();
    if( !m_directMessages.isNull() )
        delete m_directMessages.data();
    if( !m_directMessageNew.isNull() )
        delete m_directMessageNew.data();
    if( !m_directMessageDestroy.isNull() )
        delete m_directMessageDestroy.data();

    m_cachedTwitterAuth = 0;

    m_configuration[ "cachedpeers" ] = m_cachedPeers;
    syncConfig();
    m_cachedPeers.empty();
    m_state = Tomahawk::Accounts::Account::Disconnected;
    emit stateChanged( m_state );
}

void
TwitterSipPlugin::accountAuthenticated( const QPointer< TomahawkOAuthTwitter > &twitterAuth, const QTweetUser &user )
{
    Q_UNUSED( user );

    if ( !m_account->enabled() || !m_account->isAuthenticated() )
        return;

    m_cachedTwitterAuth = twitterAuth;

    m_friendsTimeline = QPointer<QTweetFriendsTimeline>( new QTweetFriendsTimeline( m_cachedTwitterAuth.data(), this ) );
    m_mentions = QPointer<QTweetMentions>( new QTweetMentions( m_cachedTwitterAuth.data(), this ) );
    m_directMessages = QPointer<QTweetDirectMessages>( new QTweetDirectMessages( m_cachedTwitterAuth.data(), this ) );
    m_directMessageNew = QPointer<QTweetDirectMessageNew>( new QTweetDirectMessageNew( m_cachedTwitterAuth.data(), this ) );
    m_directMessageDestroy = QPointer<QTweetDirectMessageDestroy>( new QTweetDirectMessageDestroy( m_cachedTwitterAuth.data(), this ) );
    connect( m_friendsTimeline.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( friendsTimelineStatuses(const QList<QTweetStatus> &) ) );
    connect( m_mentions.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( mentionsStatuses(const QList<QTweetStatus> &) ) );
    connect( m_directMessages.data(), SIGNAL( parsedDirectMessages(const QList<QTweetDMStatus> &)), SLOT( directMessages(const QList<QTweetDMStatus> &) ) );
    connect( m_directMessageNew.data(), SIGNAL( parsedDirectMessage(const QTweetDMStatus &)), SLOT( directMessagePosted(const QTweetDMStatus &) ) );
    connect( m_directMessageNew.data(), SIGNAL( error(QTweetNetBase::ErrorCode, const QString &) ), SLOT( directMessagePostError(QTweetNetBase::ErrorCode, const QString &) ) );
    connect( m_directMessageDestroy.data(), SIGNAL( parsedDirectMessage(const QTweetDMStatus &) ), SLOT( directMessageDestroyed(const QTweetDMStatus &) ) );
    m_state = Tomahawk::Accounts::Account::Connected;
    emit stateChanged( m_state );
    QStringList peerList = m_cachedPeers.keys();
    qStableSort( peerList.begin(), peerList.end() );
    registerOffers( peerList );
    m_connectTimer.start();
    m_checkTimer.start();
    m_dmPollTimer.start();

    QMetaObject::invokeMethod( this, "checkTimerFired", Qt::AutoConnection );
    QTimer::singleShot( 20000, this, SLOT( connectTimerFired() ) );
}


void
TwitterSipPlugin::checkTimerFired()
{
    if ( !isValid() )
        return;

    if ( m_cachedFriendsSinceId == 0 )
        m_cachedFriendsSinceId = m_configuration[ "cachedfriendssinceid" ].toLongLong();

    qDebug() << "TwitterSipPlugin looking at friends timeline since id " << m_cachedFriendsSinceId;

    if ( !m_friendsTimeline.isNull() )
        m_friendsTimeline.data()->fetch( m_cachedFriendsSinceId, 0, 800 );

    if ( m_cachedMentionsSinceId == 0 )
        m_cachedMentionsSinceId = m_configuration[ "cachedmentionssinceid" ].toLongLong();

    qDebug() << "TwitterSipPlugin looking at mentions timeline since id " << m_cachedMentionsSinceId;

    if ( !m_mentions.isNull() )
        m_mentions.data()->fetch( m_cachedMentionsSinceId, 0, 800 );
}


void
TwitterSipPlugin::registerOffers( const QStringList &peerList )
{
    if ( !isValid() )
        return;

    foreach( QString screenName, peerList )
    {
        QVariantHash peerData = m_cachedPeers[screenName].toHash();

        if ( peerData.contains( "onod" ) && peerData["onod"] != Database::instance()->impl()->dbid() )
        {
            m_cachedPeers.remove( screenName );
            m_configuration[ "cachedpeers" ] = m_cachedPeers;
            syncConfig();
        }

        if ( Servent::instance()->connectedToSession( peerData["node"].toString() ) )
        {
            peerData["lastseen"] = QDateTime::currentMSecsSinceEpoch();
            m_cachedPeers[screenName] = peerData;
            m_configuration[ "cachedpeers" ] = m_cachedPeers;
            syncConfig();
            qDebug() << Q_FUNC_INFO << " already connected";
            continue;
        }
        else if ( QDateTime::currentMSecsSinceEpoch() - peerData["lastseen"].toLongLong() > 1209600000 ) // 2 weeks
        {
            qDebug() << Q_FUNC_INFO << " aging peer " << screenName << " out of cache";
            m_cachedPeers.remove( screenName );
            m_configuration[ "cachedpeers" ] = m_cachedPeers;
            syncConfig();
            m_cachedAvatars.remove( screenName );
            continue;
        }

        if ( !peerData.contains( "host" ) || !peerData.contains( "port" ) || !peerData.contains( "pkey" ) )
        {
            qDebug() << "TwitterSipPlugin does not have host, port and/or pkey values for " << screenName << " (this is usually *not* a bug or problem but a normal part of the process)";
            continue;
        }

        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), Q_ARG( QVariantHash, peerData ) );
    }
}


void
TwitterSipPlugin::connectTimerFired()
{
    tDebug() << Q_FUNC_INFO << " beginning";
    if ( !isValid() || m_cachedPeers.isEmpty() )
    {
        if ( !isValid() )
            tDebug() << Q_FUNC_INFO << " is not valid";
        if ( m_cachedPeers.isEmpty() )
            tDebug() << Q_FUNC_INFO << " has empty cached peers";
        return;
    }

    tDebug() << Q_FUNC_INFO << " continuing";
    QString myScreenName = m_configuration[ "screenname" ].toString();
    QStringList peerList = m_cachedPeers.keys();
    qStableSort( peerList.begin(), peerList.end() );
    registerOffers( peerList );
}

void
TwitterSipPlugin::parseGotTomahawk( const QRegExp &regex, const QString &screenName, const QString &text )
{
    QString myScreenName = m_configuration[ "screenname" ].toString();
    qDebug() << "TwitterSipPlugin found an exact matching Got Tomahawk? mention or direct message from user " << screenName << ", now parsing";
    regex.exactMatch( text );
    if ( text.startsWith( '@' ) && regex.captureCount() >= 2 && regex.cap( 1 ) != QString( '@' + myScreenName ) )
    {
        qDebug() << "TwitterSipPlugin skipping mention because it's directed @someone that isn't us";
        return;
    }

    QString node;
    for ( int i = 0; i < regex.captureCount(); ++i )
    {
        if ( regex.cap( i ) == QString( "Got Tomahawk?" ) )
        {
            QString nodeCap = regex.cap( i + 1 );
            nodeCap.chop( 1 );
            node = nodeCap.mid( 1 );
        }
    }
    if ( node.isEmpty() )
    {
        qDebug() << "TwitterSipPlugin could not parse node out of the tweet";
        return;
    }
    else
        qDebug() << "TwitterSipPlugin parsed node " << node << " out of the tweet";

    if ( node == Database::instance()->impl()->dbid() )
    {
        qDebug() << "My dbid found; ignoring";
        return;
    }

    QVariantHash peerData;
    if( m_cachedPeers.contains( screenName ) )
    {
        peerData = m_cachedPeers[screenName].toHash();
        //force a re-send of info but no need to re-register
        peerData["resend"] = QVariant::fromValue< bool >( true );
        if ( peerData["node"].toString() != node )
            peerData["rekey"] = QVariant::fromValue< bool >( true );
    }
    peerData["node"] = QVariant::fromValue< QString >( node );
    QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), Q_ARG( QVariantHash, peerData ) );
}

void
TwitterSipPlugin::friendsTimelineStatuses( const QList< QTweetStatus > &statuses )
{
    tDebug() << Q_FUNC_INFO;
    QRegExp regex( s_gotTomahawkRegex, Qt::CaseSensitive, QRegExp::RegExp2 );

    QHash< QString, QTweetStatus > latestHash;
    foreach ( QTweetStatus status, statuses )
    {
        if ( !regex.exactMatch( status.text() ) )
            continue;

        if ( !latestHash.contains( status.user().screenName() ) )
            latestHash[status.user().screenName()] = status;
        else
        {
            if ( status.id() > latestHash[status.user().screenName()].id() )
                latestHash[status.user().screenName()] = status;
        }
    }

    foreach( QTweetStatus status, latestHash.values() )
    {
        if ( status.id() > m_cachedFriendsSinceId )
            m_cachedFriendsSinceId = status.id();

        tDebug() << "TwitterSipPlugin checking mention from " << status.user().screenName() << " with content " << status.text();
        parseGotTomahawk( regex, status.user().screenName(), status.text() );
    }

    m_configuration[ "cachedfriendssinceid" ] = m_cachedFriendsSinceId;
    syncConfig();
}

void
TwitterSipPlugin::mentionsStatuses( const QList< QTweetStatus > &statuses )
{
    tDebug() << Q_FUNC_INFO;
    QRegExp regex( s_gotTomahawkRegex, Qt::CaseSensitive, QRegExp::RegExp2 );

    QHash< QString, QTweetStatus > latestHash;
    foreach ( QTweetStatus status, statuses )
    {
        if ( !regex.exactMatch( status.text() ) )
            continue;

        if ( !latestHash.contains( status.user().screenName() ) )
            latestHash[status.user().screenName()] = status;
        else
        {
            if ( status.id() > latestHash[status.user().screenName()].id() )
                latestHash[status.user().screenName()] = status;
        }
    }

    foreach( QTweetStatus status, latestHash.values() )
    {
        if ( status.id() > m_cachedMentionsSinceId )
            m_cachedMentionsSinceId = status.id();

        tDebug() << "TwitterSipPlugin checking mention from " << status.user().screenName() << " with content " << status.text();
        parseGotTomahawk( regex, status.user().screenName(), status.text() );
    }

    m_configuration[ "cachedmentionssinceid" ] = m_cachedMentionsSinceId;
    syncConfig();
}

void
TwitterSipPlugin::pollDirectMessages()
{
    if ( !isValid() )
        return;

    if ( m_cachedDirectMessagesSinceId == 0 )
        m_cachedDirectMessagesSinceId = m_configuration[ "cacheddirectmessagessinceid" ].toLongLong();

    tDebug() << "TwitterSipPlugin looking for direct messages since id " << m_cachedDirectMessagesSinceId;

    if ( !m_directMessages.isNull() )
        m_directMessages.data()->fetch( m_cachedDirectMessagesSinceId, 0, 800 );
}

void
TwitterSipPlugin::directMessages( const QList< QTweetDMStatus > &messages )
{
    tDebug() << Q_FUNC_INFO;

    QRegExp regex( s_gotTomahawkRegex, Qt::CaseSensitive, QRegExp::RegExp2 );
    QString myScreenName = m_configuration[ "screenname" ].toString();

    QHash< QString, QTweetDMStatus > latestHash;
    foreach ( QTweetDMStatus status, messages )
    {
        if ( !regex.exactMatch( status.text() ) )
        {
            QStringList splitList = status.text().split(':');
            if ( splitList.length() != 5 )
                continue;
            if ( splitList[0] != "TOMAHAWKPEER" )
                continue;
            if ( !splitList[1].startsWith( "Host=" ) || !splitList[2].startsWith( "Port=" ) || !splitList[3].startsWith( "Node=" ) || !splitList[4].startsWith( "PKey=" ) )
                continue;
            int port = splitList[2].mid( 5 ).toInt();
            if ( port == 0 )
                continue;
        }

        if ( !latestHash.contains( status.senderScreenName() ) )
            latestHash[status.senderScreenName()] = status;
        else
        {
            if ( status.id() > latestHash[status.senderScreenName()].id() )
                latestHash[status.senderScreenName()] = status;
        }
    }

    foreach( QTweetDMStatus status, latestHash.values() )
    {
        qDebug() << "TwitterSipPlugin checking direct message from " << status.senderScreenName() << " with content " << status.text();
        if ( status.id() > m_cachedDirectMessagesSinceId )
            m_cachedDirectMessagesSinceId = status.id();

        if ( regex.exactMatch( status.text() ) )
            parseGotTomahawk( regex, status.sender().screenName(), status.text() );
        else
        {
            QStringList splitList = status.text().split(':');
            qDebug() << "TwitterSipPlugin found " << splitList.length() << " parts to the message; the parts are:";
            foreach( QString part, splitList )
                qDebug() << part;
            //validity is checked above
            int port = splitList[2].mid( 5 ).toInt();
            QString host = splitList[1].mid( 5 );
            QString node = splitList[3].mid( 5 );
            QString pkey = splitList[4].mid( 5 );
            QStringList splitNode = node.split('*');
            if ( splitNode.length() != 2 )
            {
                qDebug() << "Old-style node info found, ignoring";
                continue;
            }
            qDebug() << "TwitterSipPlugin found a peerstart message from " << status.senderScreenName() << " with host " << host << " and port " << port << " and pkey " << pkey << " and node " << splitNode[0] << " destined for node " << splitNode[1];


            QVariantHash peerData = ( m_cachedPeers.contains( status.senderScreenName() ) ) ?
                                                        m_cachedPeers[status.senderScreenName()].toHash() :
                                                        QVariantHash();

            peerData["host"] = QVariant::fromValue< QString >( host );
            peerData["port"] = QVariant::fromValue< int >( port );
            peerData["pkey"] = QVariant::fromValue< QString >( pkey );
            peerData["node"] = QVariant::fromValue< QString >( splitNode[0] );
            peerData["dirty"] = QVariant::fromValue< bool >( true );

            QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, status.senderScreenName() ), Q_ARG( QVariantHash, peerData ) );

            if ( Database::instance()->impl()->dbid().startsWith( splitNode[1] ) )
            {
                qDebug() << "TwitterSipPlugin found message destined for this node; destroying it";
                if ( !m_directMessageDestroy.isNull() )
                    m_directMessageDestroy.data()->destroyMessage( status.id() );
            }
        }
    }

    m_configuration[ "cacheddirectmessagessinceid" ] = m_cachedDirectMessagesSinceId;
    syncConfig();
}

void
TwitterSipPlugin::registerOffer( const QString &screenName, const QVariantHash &peerData )
{
    qDebug() << Q_FUNC_INFO;

    bool peersChanged = false;
    bool needToSend = false;
    bool needToAddToCache = false;

    QString friendlyName = QString( '@' + screenName );

    if ( !m_cachedAvatars.contains( screenName ) )
        QMetaObject::invokeMethod( this, "fetchAvatar", Q_ARG( QString, screenName ) );

    QVariantHash _peerData( peerData );

    if ( _peerData.contains( "dirty" ) )
    {
        peersChanged = true;
        _peerData.remove( "dirty" );
    }

    if ( _peerData.contains( "resend" ) )
    {
        needToSend = true;
        peersChanged = true;
        _peerData.remove( "resend" );
    }

    if ( !_peerData.contains( "okey" ) ||
         !_peerData.contains( "onod" ) ||
         ( _peerData.contains( "onod" ) && _peerData["onod"] != Database::instance()->impl()->dbid() ) )
    {
        QString okey = QUuid::createUuid().toString().split( '-' ).last();
        okey.chop( 1 );
        _peerData["okey"] = QVariant::fromValue< QString >( okey );
        _peerData["onod"] = QVariant::fromValue< QString >( Database::instance()->impl()->dbid() );
        peersChanged = true;
        needToAddToCache = true;
        needToSend = true;
    }

    if ( _peerData.contains( "rekey" ) || !m_keyCache.contains( _peerData["okey"].toString() ) )
    {
        _peerData.remove( "rekey" );
        needToAddToCache = true;
    }

    if ( !_peerData.contains( "ohst" ) || !_peerData.contains( "oprt" ) ||
            _peerData["ohst"].toString() != Servent::instance()->externalAddress() ||
            _peerData["oprt"].toInt() != Servent::instance()->externalPort()
        )
        needToSend = true;

    if( needToAddToCache && _peerData.contains( "node" ) )
    {
        qDebug() << "TwitterSipPlugin registering offer to " << friendlyName << " with node " << _peerData["node"].toString() << " and offeredkey " << _peerData["okey"].toString();
        m_keyCache << Servent::instance()->createConnectionKey( friendlyName, _peerData["node"].toString(), _peerData["okey"].toString(), false );
    }

    if( needToSend && _peerData.contains( "node") )
    {
        qDebug() << "TwitterSipPlugin needs to send and has node";
        _peerData["ohst"] = QVariant::fromValue< QString >( Servent::instance()->externalAddress() );
        _peerData["oprt"] = QVariant::fromValue< int >( Servent::instance()->externalPort() );
        peersChanged = true;
        if( !Servent::instance()->externalAddress().isEmpty() && !Servent::instance()->externalPort() == 0 )
            QMetaObject::invokeMethod( this, "sendOffer", Q_ARG( QString, screenName ), Q_ARG( QVariantHash, _peerData ) );
        else
            qDebug() << "TwitterSipPlugin did not send offer because external address is " << Servent::instance()->externalAddress() << " and external port is " << Servent::instance()->externalPort();
    }

    if ( peersChanged )
    {
        _peerData["lastseen"] = QString::number( QDateTime::currentMSecsSinceEpoch() );
        m_cachedPeers[screenName] = QVariant::fromValue< QVariantHash >( _peerData );
        m_configuration[ "cachedpeers" ] = m_cachedPeers;
        syncConfig();
    }

    if ( m_state == Tomahawk::Accounts::Account::Connected && _peerData.contains( "host" ) && _peerData.contains( "port" ) && _peerData.contains( "pkey" ) )
        QMetaObject::invokeMethod( this, "makeConnection", Q_ARG( QString, screenName ), Q_ARG( QVariantHash, _peerData ) );

}

void
TwitterSipPlugin::sendOffer( const QString &screenName, const QVariantHash &peerData )
{
    qDebug() << Q_FUNC_INFO;
    QString offerString = QString( "TOMAHAWKPEER:Host=%1:Port=%2:Node=%3*%4:PKey=%5" ).arg( peerData["ohst"].toString() )
                                                                                      .arg( peerData["oprt"].toString() )
                                                                                      .arg( Database::instance()->impl()->dbid() )
                                                                                      .arg( peerData["node"].toString().left( 8 ) )
                                                                                      .arg( peerData["okey"].toString() );
    qDebug() << "TwitterSipPlugin sending message to " << screenName << ": " << offerString;
    if( !m_directMessageNew.isNull() )
        m_directMessageNew.data()->post( screenName, offerString );
}

void
TwitterSipPlugin::makeConnection( const QString &screenName, const QVariantHash &peerData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !peerData.contains( "host" ) || !peerData.contains( "port" ) || !peerData.contains( "pkey" ) || !peerData.contains( "node" ) ||
         peerData["host"].toString().isEmpty() || peerData["port"].toString().isEmpty() || peerData["pkey"].toString().isEmpty() || peerData["node"].toString().isEmpty() )
    {
        qDebug() << "TwitterSipPlugin could not find host and/or port and/or pkey and/or node for peer " << screenName;
        return;
    }

    if ( peerData["host"].toString() == Servent::instance()->externalAddress() &&
         peerData["port"].toInt() == Servent::instance()->externalPort() )
    {
        qDebug() << "TwitterSipPlugin asked to make connection to our own host and port, ignoring " << screenName;
        return;
    }

    QString friendlyName = QString( '@' + screenName );
    if ( !Servent::instance()->connectedToSession( peerData["node"].toString() ) )
        Servent::instance()->connectToPeer( peerData["host"].toString(),
                                            peerData["port"].toString().toInt(),
                                            peerData["pkey"].toString(),
                                            friendlyName,
                                            peerData["node"].toString() );
}

void
TwitterSipPlugin::directMessagePosted( const QTweetDMStatus& message )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterSipPlugin sent message to " << message.recipientScreenName() << " containing: " << message.text();

}

void
TwitterSipPlugin::directMessagePostError( QTweetNetBase::ErrorCode errorCode, const QString &message )
{
    Q_UNUSED( errorCode );
    Q_UNUSED( message );
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterSipPlugin received an error posting direct message: " << m_directMessageNew.data()->lastErrorMessage();
}

void
TwitterSipPlugin::directMessageDestroyed( const QTweetDMStatus& message )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterSipPlugin destroyed message " << message.text();
}

void
TwitterSipPlugin::fetchAvatar( const QString& screenName )
{
    qDebug() << Q_FUNC_INFO;
    if ( !isValid() )
        return;

    QTweetUserShow *userShowFetch = new QTweetUserShow( m_cachedTwitterAuth.data(), this );
    connect( userShowFetch, SIGNAL( parsedUserInfo( QTweetUser ) ), SLOT( avatarUserDataSlot( QTweetUser ) ) );
    userShowFetch->fetch( screenName );
}

void
TwitterSipPlugin::avatarUserDataSlot( const QTweetUser &user )
{
    tDebug() << Q_FUNC_INFO;
    if ( !isValid() || user.profileImageUrl().isEmpty())
        return;

    QNetworkRequest request( user.profileImageUrl() );
    QNetworkReply *reply = m_cachedTwitterAuth.data()->networkAccessManager()->get( request );
    reply->setProperty( "screenname", user.screenName() );
    connect( reply, SIGNAL( finished() ), this, SLOT( profilePicReply() ) );
}


void
TwitterSipPlugin::profilePicReply()
{
    tDebug() << Q_FUNC_INFO;
    QNetworkReply *reply = qobject_cast< QNetworkReply* >( sender() );
    if ( !reply || reply->error() != QNetworkReply::NoError || !reply->property( "screenname" ).isValid() )
    {
        tDebug() << Q_FUNC_INFO << " reply not valid or came back with error";
        return;
    }
    QString screenName = reply->property( "screenname" ).toString();
    QString friendlyName = '@' + screenName;
    QByteArray rawData = reply->readAll();
    QImage image;
    image.loadFromData( rawData, "PNG" );
    QPixmap pixmap = QPixmap::fromImage( image );
    m_cachedAvatars[screenName] = pixmap;
    emit avatarReceived( friendlyName, QPixmap::fromImage( image ) );
}

void
TwitterSipPlugin::configurationChanged()
{
    tDebug() << Q_FUNC_INFO;
    if ( m_state != Tomahawk::Accounts::Account::Disconnected )
        m_account->deauthenticate();
    connectPlugin();
}


void
TwitterSipPlugin::syncConfig()
{
    m_account->setConfiguration( m_configuration );
    m_account->sync();
}
