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


TwitterPlugin::TwitterPlugin()
    : SipPlugin()
    , m_isAuthed( false )
    , m_isOnline( false )
    , m_checkTimer( this )
    , m_connectTimer( this )
    , m_cachedFriendsSinceId( 0 )
    , m_cachedMentionsSinceId( 0 )
    , m_cachedDirectMessagesSinceId( 0 )
    , m_cachedPeers()
    , m_keyCache()
    , m_finishedFriends( false )
    , m_finishedMentions( false )
    , m_configWidget( 0 )
{
    qDebug() << Q_FUNC_INFO;
    m_checkTimer.setInterval( 60000 );
    m_checkTimer.setSingleShot( false );
    connect( &m_checkTimer, SIGNAL( timeout() ), SLOT( checkTimerFired() ) );
    m_checkTimer.start();

    m_connectTimer.setInterval( 60000 );
    m_connectTimer.setSingleShot( false );
    connect( &m_connectTimer, SIGNAL( timeout() ), SLOT( connectTimerFired() ) );
    m_connectTimer.start();
}

bool
TwitterPlugin::isValid()
{
    return m_isAuthed || !m_cachedPeers.isEmpty();
}

const QString
TwitterPlugin::name()
{
    qDebug() << "TwitterPlugin returning plugin name " << QString( MYNAME );
    return QString( MYNAME );
}

const QString
TwitterPlugin::friendlyName()
{
    return QString("Twitter");
}

const QString
TwitterPlugin::accountName()
{
    return QString( TomahawkSettings::instance()->twitterScreenName() );
}

QWidget* TwitterPlugin::configWidget()
{
    if( m_configWidget )
        return m_configWidget;

    m_configWidget = new TwitterConfigWidget( this );

    return m_configWidget;
}



bool
TwitterPlugin::connectPlugin( bool /*startup*/ )
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSettings *settings = TomahawkSettings::instance();

    m_cachedPeers = settings->twitterCachedPeers();
    QList<QString> peerlist = m_cachedPeers.keys();
    qStableSort( peerlist.begin(), peerlist.end() );
    foreach( QString screenName, peerlist )
    {
        QHash< QString, QVariant > cachedPeer = m_cachedPeers[screenName].toHash();
        foreach( QString prop, cachedPeer.keys() )
            qDebug() << "TwitterPlugin : " << screenName << ", key " << prop << ", value " << ( cachedPeer[prop].canConvert< QString >() ? cachedPeer[prop].toString() : QString::number( cachedPeer[prop].toInt() ) );
        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&cachedPeer ) );
    }
    
    if ( settings->twitterOAuthToken().isEmpty() || settings->twitterOAuthTokenSecret().isEmpty() )
    {
        qDebug() << "TwitterPlugin has empty Twitter credentials; not connecting";
        return m_cachedPeers.isEmpty();
    }
 
    delete m_twitterAuth.data();
    m_twitterAuth = QWeakPointer<TomahawkOAuthTwitter>( new TomahawkOAuthTwitter( this ) );
    m_twitterAuth.data()->setNetworkAccessManager( TomahawkUtils::nam() );
    m_twitterAuth.data()->setOAuthToken( settings->twitterOAuthToken().toLatin1() );
    m_twitterAuth.data()->setOAuthTokenSecret( settings->twitterOAuthTokenSecret().toLatin1() );
    
    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
    connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( connectAuthVerifyReply(const QTweetUser &) ) );
    credVerifier->verify();
    
    return true;
}

void
TwitterPlugin::disconnectPlugin()
{
    qDebug() << Q_FUNC_INFO;
    if( !m_friendsTimeline.isNull() )
        m_friendsTimeline.data()->deleteLater();
    if( !m_twitterAuth.isNull() )
        m_twitterAuth.data()->deleteLater();
    
    m_cachedPeers.empty();
    delete m_twitterAuth.data();
    m_isOnline = false;
}

void
TwitterPlugin::connectAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        qDebug() << "TwitterPlugin could not authenticate to Twitter";
        m_isAuthed = false;
    }
    else
    {
        qDebug() << "TwitterPlugin successfully authenticated to Twitter as user " << user.screenName();
        m_isAuthed = true;
        if ( !m_twitterAuth.isNull() )
        {
            TomahawkSettings::instance()->setTwitterScreenName( user.screenName() );
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
            m_isOnline = true;
            QMetaObject::invokeMethod( this, "checkTimerFired", Qt::AutoConnection );
            QTimer::singleShot( 20000, this, SLOT( connectTimerFired() ) );
        }
        else
        {
            qDebug() << "TwitterPlugin auth pointer was null!";
            m_isAuthed = false;
        }
    }
}

void
TwitterPlugin::checkTimerFired()
{
    if ( !isValid() )
        return;

    if ( m_cachedFriendsSinceId == 0 )
        m_cachedFriendsSinceId = TomahawkSettings::instance()->twitterCachedFriendsSinceId();
    
    qDebug() << "TwitterPlugin looking at friends timeline since id " << m_cachedFriendsSinceId;
    
    if ( !m_friendsTimeline.isNull() )
        m_friendsTimeline.data()->fetch( m_cachedFriendsSinceId, 0, 800 );
    
    
    if ( m_cachedMentionsSinceId == 0 )
        m_cachedMentionsSinceId = TomahawkSettings::instance()->twitterCachedMentionsSinceId();
    
    qDebug() << "TwitterPlugin looking at mentions timeline since id " << m_cachedMentionsSinceId;
    
    if ( !m_mentions.isNull() )
        m_mentions.data()->fetch( m_cachedMentionsSinceId, 0, 800 );
}

void
TwitterPlugin::connectTimerFired()
{
    qDebug() << Q_FUNC_INFO;
    if ( !isValid() || m_cachedPeers.isEmpty() )
        return;
    
    QString myScreenName = TomahawkSettings::instance()->twitterScreenName();
    QList<QString> peerlist = m_cachedPeers.keys();
    qStableSort( peerlist.begin(), peerlist.end() );
    foreach( QString screenName, peerlist )
    {
        QHash< QString, QVariant > peerData = m_cachedPeers[screenName].toHash();
        
        if ( !peerData.contains( "host" ) || !peerData.contains( "port" ) || !peerData.contains( "pkey" ) )
        {
            qDebug() << "TwitterPlugin does not have host, port and/or pkey values for " << screenName;
            continue;
        }
        
        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
    }
}

void
TwitterPlugin::friendsTimelineStatuses( const QList< QTweetStatus > &statuses )
{
    qDebug() << Q_FUNC_INFO;
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?(Got Tomahawk\\?) (\\{[a-fA-F0-9\\-]+\\}) (.*)$" ), Qt::CaseSensitive, QRegExp::RegExp2 );
    QString myScreenName = TomahawkSettings::instance()->twitterScreenName();
    
    QHash< QString, QTweetStatus > latestHash;
    foreach ( QTweetStatus status, statuses )
    {
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

        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact tweet from friend " << status.user().screenName();
            if ( status.text().startsWith( '@' ) && regex.captureCount() >= 2 && regex.cap( 1 ) != QString( '@' + myScreenName ) )
            {
                qDebug() << "TwitterPlugin skipping tweet because it's directed @someone that isn't us";
                continue;
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
                continue;
            }
            else
                qDebug() << "TwitterPlugin parsed node " << node << " out of the tweet";
        
            if ( status.user().screenName() == myScreenName && node == Database::instance()->dbid() )
                continue;
            
            QHash< QString, QVariant > peerData;
            if( m_cachedPeers.contains( status.user().screenName() ) )
            {
                peerData = m_cachedPeers[status.user().screenName()].toHash();
                //force a re-send of info but no need to re-register
                peerData["resend"] = QVariant::fromValue< bool >( true );
            }
            peerData["node"] = QVariant::fromValue< QString >( node );
            QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, status.user().screenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
        }
    }
    
    TomahawkSettings::instance()->setTwitterCachedFriendsSinceId( m_cachedFriendsSinceId );
    
    m_finishedFriends = true;
    QMetaObject::invokeMethod( this, "pollDirectMessages", Qt::AutoConnection );
}

void
TwitterPlugin::mentionsStatuses( const QList< QTweetStatus > &statuses )
{
    qDebug() << Q_FUNC_INFO;
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?(Got Tomahawk\\?) (\\{[a-fA-F0-9\\-]+\\}) (.*)$" ), Qt::CaseSensitive, QRegExp::RegExp2 );
    QString myScreenName = TomahawkSettings::instance()->twitterScreenName();
    
    QHash< QString, QTweetStatus > latestHash;
    foreach ( QTweetStatus status, statuses )
    {
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

        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact matching mention from user " << status.user().screenName();
            if ( status.text().startsWith( '@' ) && regex.captureCount() >= 2 && regex.cap( 1 ) != QString( '@' + myScreenName ) )
            {
                qDebug() << "TwitterPlugin skipping mention because it's directed @someone that isn't us";
                continue;
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
                continue;
            }
            else
                qDebug() << "TwitterPlugin parsed node " << node << " out of the tweet";
            
            if ( status.user().screenName() == myScreenName && node == Database::instance()->dbid() )
                continue;
            
            QHash< QString, QVariant > peerData;
            if( m_cachedPeers.contains( status.user().screenName() ) )
            {
                peerData = m_cachedPeers[status.user().screenName()].toHash();
                //force a re-send of info but no need to re-register
                peerData["resend"] = QVariant::fromValue< bool >( true );
            }
            peerData["node"] = QVariant::fromValue< QString >( node );
            QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, status.user().screenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
        }
    }
    
    TomahawkSettings::instance()->setTwitterCachedMentionsSinceId( m_cachedMentionsSinceId );
    
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
            m_cachedDirectMessagesSinceId = TomahawkSettings::instance()->twitterCachedDirectMessagesSinceId();
    
    qDebug() << "TwitterPlugin looking for direct messages since id " << m_cachedDirectMessagesSinceId;
    
    if ( !m_directMessages.isNull() )
        m_directMessages.data()->fetch( m_cachedDirectMessagesSinceId, 0, 800 );
}

void
TwitterPlugin::directMessages( const QList< QTweetDMStatus > &messages )
{
    qDebug() << Q_FUNC_INFO;
    
    QHash< QString, QTweetDMStatus > latestHash;
    foreach ( QTweetDMStatus status, messages )
    {
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
        QStringList splitList = status.text().split(':');
        qDebug() << "TwitterPlugin found " << splitList.length() << " parts to the message; the parts are:";
        foreach( QString part, splitList )
            qDebug() << part;
        if ( splitList.length() != 5 )
            continue;
        if ( splitList[0] != "TOMAHAWKPEER" )
            continue;
        if ( !splitList[1].startsWith( "Host=" ) || !splitList[2].startsWith( "Port=" ) || !splitList[3].startsWith( "Node=" ) || !splitList[4].startsWith( "PKey=" ) )
            continue;
        int port = splitList[2].mid( 5 ).toInt();
        if ( port == 0 )
            continue;
        QString host = splitList[1].mid( 5 );
        QString node = splitList[3].mid( 5 );
        QString pkey = splitList[4].mid( 5 );
        qDebug() << "TwitterPlugin found a peerstart message from " << status.senderScreenName() << " with host " << host << " and port " << port << " and pkey " << pkey << " destined for node " << node;
        
        if ( node != Database::instance()->dbid() )
        {
            qDebug() << "Not destined for this node; leaving it alone and not answering";
            continue;
        }
        
        QHash< QString, QVariant > peerData = ( m_cachedPeers.contains( status.senderScreenName() ) ) ?
                                                    m_cachedPeers[status.senderScreenName()].toHash() :
                                                    QHash< QString, QVariant >();
        
        peerData["host"] = QVariant::fromValue< QString >( host );
        peerData["port"] = QVariant::fromValue< int >( port );
        peerData["pkey"] = QVariant::fromValue< QString >( pkey );
        peerData["dirty"] = QVariant::fromValue< bool >( true );

        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, status.senderScreenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
        
        if ( !m_directMessageDestroy.isNull() )
            m_directMessageDestroy.data()->destroyMessage( status.id() );
    }

    TomahawkSettings::instance()->setTwitterCachedDirectMessagesSinceId( m_cachedDirectMessagesSinceId );
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

    if ( !_peerData.contains( "okey" ) )
    {
        QString okey = QUuid::createUuid().toString().split( '-' ).last();
        okey.chop( 1 );
        _peerData["okey"] = QVariant::fromValue< QString >( okey );
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

    if ( m_isOnline && _peerData.contains( "host" ) && _peerData.contains( "port" ) && _peerData.contains( "pkey" ) )
        QMetaObject::invokeMethod( this, "makeConnection", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&_peerData ) );
    
    if ( peersChanged )
    {
        m_cachedPeers[screenName] = QVariant::fromValue< QHash< QString, QVariant > >( _peerData );
        TomahawkSettings::instance()->setTwitterCachedPeers( m_cachedPeers );
    }
}

void
TwitterPlugin::sendOffer( const QString &screenName, const QHash< QString, QVariant > &peerData )
{
    qDebug() << Q_FUNC_INFO;
    QString offerString = QString( "TOMAHAWKPEER:Host=%1:Port=%2:Node=%3:PKey=%4" ).arg( peerData["ohst"].toString() )
                                                                                   .arg( peerData["oprt"].toString() )
                                                                                   .arg( peerData["node"].toString() )
                                                                                   .arg( peerData["okey"].toString() );
    qDebug() << "TwitterPlugin sending message to " << screenName << ": " << offerString;
    if( !m_directMessageNew.isNull() )
        m_directMessageNew.data()->post( screenName, offerString );
}

void
TwitterPlugin::makeConnection( const QString &screenName, const QHash< QString, QVariant > &peerData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !peerData.contains( "host" ) || !peerData.contains( "port" ) || !peerData.contains( "pkey" ) || !peerData.contains( "node" ) )
    {
        qDebug() << "TwitterPlugin could not find host and/or port and/or pkey for peer " << screenName;
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
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterPlugin received an error posting direct message: " << m_directMessageNew.data()->lastErrorMessage();
}

void
TwitterPlugin::directMessageDestroyed( const QTweetDMStatus& message )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "TwitterPlugin destroyed message " << message.text();
}

Q_EXPORT_PLUGIN2( sip, TwitterPlugin )
