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

#include "twitter.h"

#include "twitterconfigwidget.h"

#include <QtPlugin>
#include <QRegExp>
#include <QStringList>

#include <qtweetaccountverifycredentials.h>
#include <qtweetuser.h>
#include <qtweetstatus.h>

#include <utils/tomahawkutils.h>
#include <tomahawksettings.h>
#include <database/database.h>
#include <network/servent.h>

static QString s_gotTomahawkRegex = QString( "^(@[a-zA-Z0-9]+ )?(Got Tomahawk\\?) (\\{[a-fA-F0-9\\-]+\\}) (.*)$" );

SipPlugin*
TwitterFactory::createPlugin( const QString& pluginId )
{
    return new TwitterPlugin( pluginId.isEmpty() ? generateId() : pluginId );
}

QIcon TwitterFactory::icon() const
{
    return QIcon( ":/twitter-icon.png" );
}


TwitterPlugin::TwitterPlugin( const QString& pluginId )
    : SipPlugin( pluginId )
    , m_isAuthed( false )
    , m_checkTimer( this )
    , m_connectTimer( this )
    , m_cachedFriendsSinceId( 0 )
    , m_cachedMentionsSinceId( 0 )
    , m_cachedDirectMessagesSinceId( 0 )
    , m_cachedPeers()
    , m_keyCache()
    , m_finishedFriends( false )
    , m_finishedMentions( false )
    , m_state( Disconnected )
{
    qDebug() << Q_FUNC_INFO;
    m_checkTimer.setInterval( 60000 );
    m_checkTimer.setSingleShot( false );
    connect( &m_checkTimer, SIGNAL( timeout() ), SLOT( checkTimerFired() ) );

    m_connectTimer.setInterval( 60000 );
    m_connectTimer.setSingleShot( false );
    connect( &m_connectTimer, SIGNAL( timeout() ), SLOT( connectTimerFired() ) );

    m_configWidget = QWeakPointer< TwitterConfigWidget >( new TwitterConfigWidget( this, 0 ) );
    connect( m_configWidget.data(), SIGNAL( twitterAuthed( bool ) ), SLOT( configDialogAuthedSignalSlot( bool ) ) );

}

void
TwitterPlugin::configDialogAuthedSignalSlot( bool authed )
{

    if ( !authed )
    {
        if( m_isAuthed ) {
            m_state = Disconnected;
            emit stateChanged( m_state );
        }

        setTwitterScreenName( QString() );
        setTwitterOAuthToken( QString() );
        setTwitterOAuthTokenSecret( QString() );
    }

    m_isAuthed = authed;
}

bool
TwitterPlugin::isValid() const
{
    return m_isAuthed;
}

const QString
TwitterPlugin::name() const
{
    return QString( MYNAME );
}

const QString
TwitterPlugin::friendlyName() const
{
    return tr("Twitter");
}

const QString
TwitterPlugin::accountName() const
{
    if( twitterScreenName().isEmpty() )
        return friendlyName();
    else
        return twitterScreenName();
}

QIcon
TwitterPlugin::icon() const
{
    return QIcon( ":/twitter-icon.png" );
}


SipPlugin::ConnectionState
TwitterPlugin::connectionState() const
{
    return m_state;
}


QWidget* TwitterPlugin::configWidget()
{
    return m_configWidget.data();
}

bool
TwitterPlugin::connectPlugin( bool startup )
{
    qDebug() << Q_FUNC_INFO;

    if( startup && !twitterAutoConnect() )
        return false;

    m_cachedPeers = twitterCachedPeers();
    QList<QString> peerlist = m_cachedPeers.keys();
    qStableSort( peerlist.begin(), peerlist.end() );
    foreach( QString screenName, peerlist )
    {
        QHash< QString, QVariant > cachedPeer = m_cachedPeers[screenName].toHash();
        foreach( QString prop, cachedPeer.keys() )
            qDebug() << "TwitterPlugin : " << screenName << ", key " << prop << ", value " << ( cachedPeer[prop].canConvert< QString >() ? cachedPeer[prop].toString() : QString::number( cachedPeer[prop].toInt() ) );
        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&cachedPeer ) );
    }

    if ( twitterOAuthToken().isEmpty() || twitterOAuthTokenSecret().isEmpty() )
    {
        qDebug() << "TwitterPlugin has empty Twitter credentials; not connecting";
        return m_cachedPeers.isEmpty();
    }

    if ( refreshTwitterAuth() )
    {
      QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
      connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( connectAuthVerifyReply(const QTweetUser &) ) );
      credVerifier->verify();

      m_state = Connecting;
      emit stateChanged( m_state );
    }

    return true;
}

bool
TwitterPlugin::refreshTwitterAuth()
{
    qDebug() << Q_FUNC_INFO << " begin";
    if( !m_twitterAuth.isNull() )
        delete m_twitterAuth.data();

    Q_ASSERT( TomahawkUtils::nam() != 0 );
    qDebug() << Q_FUNC_INFO << " with nam " << TomahawkUtils::nam();
    m_twitterAuth = QWeakPointer<TomahawkOAuthTwitter>( new TomahawkOAuthTwitter( TomahawkUtils::nam(), this ) );

    if( m_twitterAuth.isNull() )
      return false;

    m_twitterAuth.data()->setOAuthToken( twitterOAuthToken().toLatin1() );
    m_twitterAuth.data()->setOAuthTokenSecret( twitterOAuthTokenSecret().toLatin1() );

    return true;
}

void
TwitterPlugin::disconnectPlugin()
{
    qDebug() << Q_FUNC_INFO;
    m_checkTimer.stop();
    m_connectTimer.stop();
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
    if( !m_twitterAuth.isNull() )
        delete m_twitterAuth.data();

    m_cachedPeers.empty();
    m_state = Disconnected;
    emit stateChanged( m_state );
}

void
TwitterPlugin::connectAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        qDebug() << "TwitterPlugin could not authenticate to Twitter";
        m_isAuthed = false;
        m_state = Disconnected;
        emit stateChanged( m_state );
    }
    else
    {
        qDebug() << "TwitterPlugin successfully authenticated to Twitter as user " << user.screenName();
        m_isAuthed = true;
        if ( !m_twitterAuth.isNull() )
        {
            setTwitterScreenName( user.screenName() );
            m_friendsTimeline = QWeakPointer<QTweetFriendsTimeline>( new QTweetFriendsTimeline( m_twitterAuth.data(), this ) );
            m_mentions = QWeakPointer<QTweetMentions>( new QTweetMentions( m_twitterAuth.data(), this ) );
            m_directMessages = QWeakPointer<QTweetDirectMessages>( new QTweetDirectMessages( m_twitterAuth.data(), this ) );
            m_directMessageNew = QWeakPointer<QTweetDirectMessageNew>( new QTweetDirectMessageNew( m_twitterAuth.data(), this ) );
            m_directMessageDestroy = QWeakPointer<QTweetDirectMessageDestroy>( new QTweetDirectMessageDestroy( m_twitterAuth.data(), this ) );
            connect( m_friendsTimeline.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( friendsTimelineStatuses(const QList<QTweetStatus> &) ) );
            connect( m_mentions.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( mentionsStatuses(const QList<QTweetStatus> &) ) );
            connect( m_directMessages.data(), SIGNAL( parsedDirectMessages(const QList<QTweetDMStatus> &)), SLOT( directMessages(const QList<QTweetDMStatus> &) ) );
            connect( m_directMessageNew.data(), SIGNAL( parsedDirectMessage(const QTweetDMStatus &)), SLOT( directMessagePosted(const QTweetDMStatus &) ) );
            connect( m_directMessageNew.data(), SIGNAL( error(QTweetNetBase::ErrorCode, const QString &) ), SLOT( directMessagePostError(QTweetNetBase::ErrorCode, const QString &) ) );
            connect( m_directMessageDestroy.data(), SIGNAL( parsedDirectMessage(const QTweetDMStatus &) ), SLOT( directMessageDestroyed(const QTweetDMStatus &) ) );
            m_state = Connected;
            emit stateChanged( m_state );
            m_connectTimer.start();
            m_checkTimer.start();
            QMetaObject::invokeMethod( this, "checkTimerFired", Qt::AutoConnection );
            QTimer::singleShot( 20000, this, SLOT( connectTimerFired() ) );
        }
        else
        {
            if ( refreshTwitterAuth() )
            {
                QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
                connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( connectAuthVerifyReply(const QTweetUser &) ) );
                credVerifier->verify();
            }
            else
            {
                qDebug() << "TwitterPlugin auth pointer was null!";
                m_isAuthed = false;
                m_state = Disconnected;
                emit stateChanged( m_state );
            }
        }
    }
}

void
TwitterPlugin::checkTimerFired()
{
    if ( !isValid() || m_twitterAuth.isNull() )
        return;

    if ( m_cachedFriendsSinceId == 0 )
        m_cachedFriendsSinceId = twitterCachedFriendsSinceId();

    qDebug() << "TwitterPlugin looking at friends timeline since id " << m_cachedFriendsSinceId;

    if ( !m_friendsTimeline.isNull() )
        m_friendsTimeline.data()->fetch( m_cachedFriendsSinceId, 0, 800 );

    if ( m_cachedMentionsSinceId == 0 )
        m_cachedMentionsSinceId = twitterCachedMentionsSinceId();

    qDebug() << "TwitterPlugin looking at mentions timeline since id " << m_cachedMentionsSinceId;

    if ( !m_mentions.isNull() )
        m_mentions.data()->fetch( m_cachedMentionsSinceId, 0, 800 );
}

void
TwitterPlugin::connectTimerFired()
{
    if ( !isValid() || m_cachedPeers.isEmpty() || m_twitterAuth.isNull() )
        return;

    QString myScreenName = twitterScreenName();
    QList<QString> peerlist = m_cachedPeers.keys();
    qStableSort( peerlist.begin(), peerlist.end() );
    foreach( QString screenName, peerlist )
    {
        QHash< QString, QVariant > peerData = m_cachedPeers[screenName].toHash();

        if ( Servent::instance()->connectedToSession( peerData["node"].toString() ) )
        {
            peerData["lastseen"] = QDateTime::currentMSecsSinceEpoch();
            m_cachedPeers[screenName] = peerData;
            continue;
        }

        if ( QDateTime::currentMSecsSinceEpoch() - peerData["lastseen"].toLongLong() > 1209600000 ) // 2 weeks
        {
            qDebug() << "Aging peer " << screenName << " out of cache";
            m_cachedPeers.remove( screenName );
            continue;
        }

        if ( !peerData.contains( "host" ) || !peerData.contains( "port" ) || !peerData.contains( "pkey" ) )
        {
            qDebug() << "TwitterPlugin does not have host, port and/or pkey values for " << screenName << " (this is usually *not* a bug or problem but a normal part of the process)";
            continue;
        }

        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
    }
}

void
TwitterPlugin::parseGotTomahawk( const QRegExp &regex, const QString &screenName, const QString &text )
{
    QString myScreenName = twitterScreenName();
    qDebug() << "TwitterPlugin found an exact matching Got Tomahawk? mention or direct message from user " << screenName << ", now parsing";
    regex.exactMatch( text );
    if ( text.startsWith( '@' ) && regex.captureCount() >= 2 && regex.cap( 1 ) != QString( '@' + myScreenName ) )
    {
        qDebug() << "TwitterPlugin skipping mention because it's directed @someone that isn't us";
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
        qDebug() << "TwitterPlugin could not parse node out of the tweet";
        return;
    }
    else
        qDebug() << "TwitterPlugin parsed node " << node << " out of the tweet";

    if ( screenName == myScreenName && node == Database::instance()->dbid() )
    {
        qDebug() << "My screen name and my dbid found; ignoring";
        return;
    }

    QHash< QString, QVariant > peerData;
    if( m_cachedPeers.contains( screenName ) )
    {
        peerData = m_cachedPeers[screenName].toHash();
        //force a re-send of info but no need to re-register
        peerData["resend"] = QVariant::fromValue< bool >( true );
    }
    peerData["node"] = QVariant::fromValue< QString >( node );
    QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
}

void
TwitterPlugin::friendsTimelineStatuses( const QList< QTweetStatus > &statuses )
{
    qDebug() << Q_FUNC_INFO;
    QRegExp regex( s_gotTomahawkRegex, Qt::CaseSensitive, QRegExp::RegExp2 );
    QString myScreenName = twitterScreenName();

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

        qDebug() << "TwitterPlugin checking mention from " << status.user().screenName() << " with content " << status.text();
        parseGotTomahawk( regex, status.user().screenName(), status.text() );
    }

    setTwitterCachedFriendsSinceId( m_cachedFriendsSinceId );

    m_finishedFriends = true;
    QMetaObject::invokeMethod( this, "pollDirectMessages", Qt::AutoConnection );
}

void
TwitterPlugin::mentionsStatuses( const QList< QTweetStatus > &statuses )
{
    qDebug() << Q_FUNC_INFO;
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

        qDebug() << "TwitterPlugin checking mention from " << status.user().screenName() << " with content " << status.text();
        parseGotTomahawk( regex, status.user().screenName(), status.text() );
    }

    setTwitterCachedMentionsSinceId( m_cachedMentionsSinceId );

    m_finishedMentions = true;
    QMetaObject::invokeMethod( this, "pollDirectMessages", Qt::AutoConnection );
}

void
TwitterPlugin::pollDirectMessages()
{
    if ( !m_finishedMentions || !m_finishedFriends )
        return;

    m_finishedFriends = false;
    m_finishedMentions = false;

    if ( !isValid() )
        return;

    if ( m_cachedDirectMessagesSinceId == 0 )
            m_cachedDirectMessagesSinceId = twitterCachedDirectMessagesSinceId();

    qDebug() << "TwitterPlugin looking for direct messages since id " << m_cachedDirectMessagesSinceId;

    if ( !m_directMessages.isNull() )
        m_directMessages.data()->fetch( m_cachedDirectMessagesSinceId, 0, 800 );
}

void
TwitterPlugin::directMessages( const QList< QTweetDMStatus > &messages )
{
    qDebug() << Q_FUNC_INFO;

    QRegExp regex( s_gotTomahawkRegex, Qt::CaseSensitive, QRegExp::RegExp2 );
    QString myScreenName = twitterScreenName();

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
        qDebug() << "TwitterPlugin checking direct message from " << status.senderScreenName() << " with content " << status.text();
        if ( status.id() > m_cachedDirectMessagesSinceId )
            m_cachedDirectMessagesSinceId = status.id();

        if ( regex.exactMatch( status.text() ) )
            parseGotTomahawk( regex, status.sender().screenName(), status.text() );
        else
        {
            QStringList splitList = status.text().split(':');
            qDebug() << "TwitterPlugin found " << splitList.length() << " parts to the message; the parts are:";
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
            qDebug() << "TwitterPlugin found a peerstart message from " << status.senderScreenName() << " with host " << host << " and port " << port << " and pkey " << pkey << " and node " << splitNode[0] << " destined for node " << splitNode[1];


            QHash< QString, QVariant > peerData = ( m_cachedPeers.contains( status.senderScreenName() ) ) ?
                                                        m_cachedPeers[status.senderScreenName()].toHash() :
                                                        QHash< QString, QVariant >();

            peerData["host"] = QVariant::fromValue< QString >( host );
            peerData["port"] = QVariant::fromValue< int >( port );
            peerData["pkey"] = QVariant::fromValue< QString >( pkey );
            peerData["node"] = QVariant::fromValue< QString >( splitNode[0] );
            peerData["dirty"] = QVariant::fromValue< bool >( true );

            QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, status.senderScreenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );

            if ( Database::instance()->dbid().startsWith( splitNode[1] ) )
            {
                qDebug() << "TwitterPlugin found message destined for this node; destroying it";
                if ( !m_directMessageDestroy.isNull() )
                    m_directMessageDestroy.data()->destroyMessage( status.id() );
            }
        }
    }

    setTwitterCachedDirectMessagesSinceId( m_cachedDirectMessagesSinceId );
}

void
TwitterPlugin::registerOffer( const QString &screenName, const QHash< QString, QVariant > &peerData )
{
    qDebug() << Q_FUNC_INFO;

    bool peersChanged = false;
    bool needToSend = false;
    bool needToAddToCache = false;

    QString friendlyName = QString( '@' + screenName );

    QHash< QString, QVariant > _peerData( peerData );

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
         ( _peerData.contains( "onod" ) && _peerData["onod"] != Database::instance()->dbid() ) )
    {
        QString okey = QUuid::createUuid().toString().split( '-' ).last();
        okey.chop( 1 );
        _peerData["okey"] = QVariant::fromValue< QString >( okey );
        _peerData["onod"] = QVariant::fromValue< QString >( Database::instance()->dbid() );
        peersChanged = true;
        needToAddToCache = true;
        needToSend = true;
    }

    if ( !m_keyCache.contains( _peerData["okey"].toString() ) )
        needToAddToCache = true;

    if ( !_peerData.contains( "ohst" ) || !_peerData.contains( "oprt" ) ||
            _peerData["ohst"].toString() != Servent::instance()->externalAddress() ||
            _peerData["oprt"].toInt() != Servent::instance()->externalPort()
        )
        needToSend = true;

    if( needToAddToCache && _peerData.contains( "node" ) )
    {
        qDebug() << "TwitterPlugin registering offer to " << friendlyName << " with node " << _peerData["node"].toString() << " and offeredkey " << _peerData["okey"].toString();
        m_keyCache << Servent::instance()->createConnectionKey( friendlyName, _peerData["node"].toString(), _peerData["okey"].toString(), false );
    }

    if( needToSend && _peerData.contains( "node") )
    {
        qDebug() << "TwitterPlugin needs to send and has node";
        _peerData["ohst"] = QVariant::fromValue< QString >( Servent::instance()->externalAddress() );
        _peerData["oprt"] = QVariant::fromValue< int >( Servent::instance()->externalPort() );
        peersChanged = true;
        if( !Servent::instance()->externalAddress().isEmpty() && !Servent::instance()->externalPort() == 0 )
            QMetaObject::invokeMethod( this, "sendOffer", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&_peerData ) );
        else
            qDebug() << "TwitterPlugin did not send offer because external address is " << Servent::instance()->externalAddress() << " and external port is " << Servent::instance()->externalPort();
    }

    if ( peersChanged )
    {
        _peerData["lastseen"] = QString::number( QDateTime::currentMSecsSinceEpoch() );
        m_cachedPeers[screenName] = QVariant::fromValue< QHash< QString, QVariant > >( _peerData );
        setTwitterCachedPeers( m_cachedPeers );
    }

    if ( m_state == Connected && _peerData.contains( "host" ) && _peerData.contains( "port" ) && _peerData.contains( "pkey" ) )
        QMetaObject::invokeMethod( this, "makeConnection", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&_peerData ) );

}

void
TwitterPlugin::sendOffer( const QString &screenName, const QHash< QString, QVariant > &peerData )
{
    qDebug() << Q_FUNC_INFO;
    QString offerString = QString( "TOMAHAWKPEER:Host=%1:Port=%2:Node=%3*%4:PKey=%5" ).arg( peerData["ohst"].toString() )
                                                                                      .arg( peerData["oprt"].toString() )
                                                                                      .arg( Database::instance()->dbid() )
                                                                                      .arg( peerData["node"].toString().left( 8 ) )
                                                                                      .arg( peerData["okey"].toString() );
    qDebug() << "TwitterPlugin sending message to " << screenName << ": " << offerString;
    if( !m_directMessageNew.isNull() )
        m_directMessageNew.data()->post( screenName, offerString );
}

void
TwitterPlugin::makeConnection( const QString &screenName, const QHash< QString, QVariant > &peerData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !peerData.contains( "host" ) || !peerData.contains( "port" ) || !peerData.contains( "pkey" ) || !peerData.contains( "node" ) ||
         peerData["host"].toString().isEmpty() || peerData["port"].toString().isEmpty() || peerData["pkey"].toString().isEmpty() || peerData["node"].toString().isEmpty() )
    {
        qDebug() << "TwitterPlugin could not find host and/or port and/or pkey and/or node for peer " << screenName;
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
TwitterPlugin::directMessagePosted( const QTweetDMStatus& message )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterPlugin sent message to " << message.recipientScreenName() << " containing: " << message.text();

}

void
TwitterPlugin::directMessagePostError( QTweetNetBase::ErrorCode errorCode, const QString &message )
{
    Q_UNUSED( errorCode );
    Q_UNUSED( message );
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterPlugin received an error posting direct message: " << m_directMessageNew.data()->lastErrorMessage();
}

void
TwitterPlugin::directMessageDestroyed( const QTweetDMStatus& message )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterPlugin destroyed message " << message.text();
}

void
TwitterPlugin::checkSettings()
{
    disconnectPlugin();
    connectPlugin( false );
}


QString
TwitterPlugin::twitterScreenName() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/ScreenName" ).toString();
}

void
TwitterPlugin::setTwitterScreenName( const QString& screenName )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/ScreenName", screenName );
}

QString
TwitterPlugin::twitterOAuthToken() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/OAuthToken" ).toString();
}

void
TwitterPlugin::setTwitterOAuthToken( const QString& oauthtoken )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/OAuthToken", oauthtoken );
}

QString
TwitterPlugin::twitterOAuthTokenSecret() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/OAuthTokenSecret" ).toString();
}

void
TwitterPlugin::setTwitterOAuthTokenSecret( const QString& oauthtokensecret )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/OAuthTokenSecret", oauthtokensecret );
}

qint64
TwitterPlugin::twitterCachedFriendsSinceId() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/CachedFriendsSinceID", 0 ).toLongLong();
}

void
TwitterPlugin::setTwitterCachedFriendsSinceId( qint64 cachedId )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/CachedFriendsSinceID", cachedId );
}

qint64
TwitterPlugin::twitterCachedMentionsSinceId() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/CachedMentionsSinceID", 0 ).toLongLong();
}

void
TwitterPlugin::setTwitterCachedMentionsSinceId( qint64 cachedId )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/CachedMentionsSinceID", cachedId );
}

qint64
TwitterPlugin::twitterCachedDirectMessagesSinceId() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/CachedDirectMessagesSinceID", 0 ).toLongLong();
}

void
TwitterPlugin::setTwitterCachedDirectMessagesSinceId( qint64 cachedId )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/CachedDirectMessagesSinceID", cachedId );
}

QHash<QString, QVariant>
TwitterPlugin::twitterCachedPeers() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/CachedPeers", QHash<QString, QVariant>() ).toHash();
}

void
TwitterPlugin::setTwitterCachedPeers( const QHash<QString, QVariant> &cachedPeers )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/CachedPeers", cachedPeers );
}

void
TwitterPlugin::setTwitterAutoConnect( bool autoConnect )
{
    TomahawkSettings::instance()->setValue( pluginId() + "/AutoConnect", autoConnect );
}

bool
TwitterPlugin::twitterAutoConnect() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/AutoConnect", true ).toBool();
}


Q_EXPORT_PLUGIN2( sipfactory, TwitterFactory )
