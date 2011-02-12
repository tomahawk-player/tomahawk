#include "twitter.h"

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
    , m_checkTimer( this )
    , m_cachedFriendsSinceId( 0 )
    , m_cachedMentionsSinceId( 0 )
    , m_cachedDirectMessagesSinceId( 0 )
    , m_cachedPeers()
    , m_finishedFriends( false )
    , m_finishedMentions( false )
{
    qDebug() << Q_FUNC_INFO;
    m_checkTimer.setInterval( 60000 );
    m_checkTimer.setSingleShot( false );
    connect( &m_checkTimer, SIGNAL( timeout() ), SLOT( checkTimerFired() ) );
    m_checkTimer.start();
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
    
    TomahawkSettings::instance()->setTwitterCachedPeers( m_cachedPeers );
    m_cachedPeers.empty();
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
            m_friendsTimeline = QWeakPointer<QTweetFriendsTimeline>( new QTweetFriendsTimeline( m_twitterAuth.data(), this ) );
            m_mentions = QWeakPointer<QTweetMentions>( new QTweetMentions( m_twitterAuth.data(), this ) );
            m_directMessages = QWeakPointer<QTweetDirectMessages>( new QTweetDirectMessages( m_twitterAuth.data(), this ) );
            m_directMessageNew = QWeakPointer<QTweetDirectMessageNew>( new QTweetDirectMessageNew( m_twitterAuth.data(), this ) );
            connect( m_friendsTimeline.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( friendsTimelineStatuses(const QList<QTweetStatus> &) ) );
            connect( m_mentions.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( mentionsStatuses(const QList<QTweetStatus> &) ) );
            connect( m_directMessages.data(), SIGNAL( parsedStatuses(const QList< QTweetDMStatus > &) ), SLOT( directMessages(const QList<QTweetDMStatus> &) ) );
            connect( m_directMessageNew.data(), SIGNAL( parsedStatuses(const QList< QTweetDMStatus > &) ), SLOT( directMessageNew(const QList<QTweetDMStatus> &) ) );
            QMetaObject::invokeMethod( this, "checkTimerFired", Qt::AutoConnection );
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
    qDebug() << "TwitterPlugin using friend timeline id of " << m_cachedFriendsSinceId;
    if ( !m_friendsTimeline.isNull() )
        m_friendsTimeline.data()->fetch( m_cachedFriendsSinceId, 0, 800 );
    
    if ( m_cachedMentionsSinceId == 0 )
        m_cachedMentionsSinceId = TomahawkSettings::instance()->twitterCachedMentionsSinceId();
    qDebug() << "TwitterPlugin using mentions timeline id of " << m_cachedMentionsSinceId;
    if ( !m_mentions.isNull() )
        m_mentions.data()->fetch( m_cachedMentionsSinceId, 0, 800 );
}

void
TwitterPlugin::friendsTimelineStatuses( const QList< QTweetStatus > &statuses )
{
    qDebug() << Q_FUNC_INFO;
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?Got Tomahawk\\?(.*)$" ) );
    bool peersChanged = false;
    foreach( QTweetStatus status, statuses )
    {
        if ( status.id() > m_cachedFriendsSinceId )
            m_cachedFriendsSinceId = status.id();
        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact tweet from friend " << status.user().screenName();
            QHash<QString, QVariant> peerData;
            if ( !m_cachedPeers.contains( status.user().screenName() ) )
            {
                QHash< QString, QVariant > peerData;
                QString okey = QUuid::createUuid().toString().split( '-' ).last();
                okey.chop( 1 );
                peerData["okey"] = QVariant::fromValue< QString >( okey );
                QMetaObject::invokeMethod( this, "sendOffer", Q_ARG( QString, status.user().screenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
                peersChanged = true;
            }
        }
    }
    
    TomahawkSettings::instance()->setTwitterCachedFriendsSinceId( m_cachedFriendsSinceId );
    if ( peersChanged )
        TomahawkSettings::instance()->setTwitterCachedPeers( m_cachedPeers );
    
    m_finishedFriends = true;
    QMetaObject::invokeMethod( this, "pollDirectMessages", Qt::AutoConnection );
}

void
TwitterPlugin::mentionsStatuses( const QList< QTweetStatus > &statuses )
{
    qDebug() << Q_FUNC_INFO;
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?Got Tomahawk\\?(.*)$" ) );
    bool peersChanged = false;
    foreach( QTweetStatus status, statuses )
    {
        if ( status.id() > m_cachedMentionsSinceId )
            m_cachedMentionsSinceId = status.id();
        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact matching mention from user " << status.user().screenName();
            if ( !m_cachedPeers.contains( status.user().screenName() ) )
            {
                QHash< QString, QVariant > peerData;
                QString okey = QUuid::createUuid().toString().split( '-' ).last();
                okey.chop( 1 );
                peerData["okey"] = QVariant::fromValue< QString >( okey );
                QMetaObject::invokeMethod( this, "sendOffer", Q_ARG( QString, status.user().screenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
                peersChanged = true;
            }
        }
    }
    
    TomahawkSettings::instance()->setTwitterCachedMentionsSinceId( m_cachedMentionsSinceId );
    if ( peersChanged )
        TomahawkSettings::instance()->setTwitterCachedPeers( m_cachedPeers );
    
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
    qDebug() << "TwitterPlugin using direct messages id of " << m_cachedDirectMessagesSinceId;
    if ( !m_directMessages.isNull() )
        m_directMessages.data()->fetch( m_cachedDirectMessagesSinceId, 0, 800 );
}

void
TwitterPlugin::directMessages( const QList< QTweetDMStatus > &messages )
{
    qDebug() << Q_FUNC_INFO;
    
    bool peersChanged = false;
    
    foreach( QTweetDMStatus status, messages )
    {
        if ( status.id() > m_cachedDirectMessagesSinceId )
            m_cachedDirectMessagesSinceId = status.id();
        QStringList splitList = status.text().split(':');
        if ( splitList.length() < 5 )
            continue;
        if ( splitList[0] != "TOMAHAWKPEERSTART" )
            continue;
        if ( !splitList[1].startsWith( "Host=" ) || !splitList[2].startsWith( "Port=" ) || !splitList[3].startsWith( "Node=" ) || !splitList[4].startsWith( "PKey=" ) )
            continue;
        int port = splitList[2].mid( 5 ).toInt();
        if ( port == 0 )
            continue;
        QString host = splitList[1].mid( 5 );
        QString node = splitList[3].mid( 5 );
        QString pkey = splitList[4].mid( 5 );
        qDebug() << "TwitterPlugin found a peerstart message from " << status.senderScreenName() << " with host " << host << " and port " << port << " and node " << node << " and pkey " << pkey;
        
        QHash< QString, QVariant > peerData;
        if ( m_cachedPeers.contains( status.senderScreenName() ) )
            peerData = m_cachedPeers[status.senderScreenName()].toHash();        
        
        peerData["host"] = QVariant::fromValue< QString >( host );
        peerData["port"] = QVariant::fromValue< int >( port );
        peerData["node"] = QVariant::fromValue< QString >( node );
        peerData["pkey"] = QVariant::fromValue< QString >( pkey );
        m_cachedPeers[status.senderScreenName()] = QVariant::fromValue< QHash< QString, QVariant > >( peerData );
        peersChanged = true;
        
        QMetaObject::invokeMethod( this, "registerOffer", Q_ARG( QString, status.senderScreenName() ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerData ) );
    }
    
    TomahawkSettings::instance()->setTwitterCachedDirectMessagesSinceId( m_cachedDirectMessagesSinceId );
    if ( peersChanged )
        TomahawkSettings::instance()->setTwitterCachedPeers( m_cachedPeers );
}

void
TwitterPlugin::makeConnection( const QString &screenName, const QHash< QString, QVariant > &peerdata )
{
    qDebug() << Q_FUNC_INFO;
    if ( !peerdata.contains( "node" ) || !peerdata.contains( "host" ) || !peerdata.contains( "port" ) || !peerdata.contains( "pkey" ) )
    {
        qDebug() << "Could not find node and/or host and/or port and/or pkey for peer " << screenName;
        return;
    }
    if ( !Servent::instance()->connectedToSession( peerdata["node"].toString() ) )
        Servent::instance()->connectToPeer( peerdata["host"].toString(),
                                            peerdata["port"].toString().toInt(),
                                            peerdata["pkey"].toString(),
                                            QString( '@' + peerdata["screenname"].toString() ),
                                            peerdata["node"].toString() );
}

void
TwitterPlugin::registerOffer( const QString &screenName, const QHash< QString, QVariant > &peerdata )
{
    qDebug() << Q_FUNC_INFO;
    if ( !peerdata.contains( "node" ) || !peerdata.contains( "okey" ) )
    {
        qDebug() << "Could not find node and/or okey for peer " << screenName;
        return;
    }
    qDebug() << "TwitterPlugin registering offer to " << QString( '@' + screenName ) << " with node " << peerdata["node"].toString() << " and offeredkey " << peerdata["okey"].toString();
    Servent::instance()->createConnectionKey( QString( '@' + screenName ), peerdata["node"].toString(), peerdata["okey"].toString() );
    if ( peerdata.contains( "node" ) && peerdata.contains( "host" ) && peerdata.contains( "port" ) && peerdata.contains( "pkey" ) )
        QMetaObject::invokeMethod( this, "makeConnection", Q_ARG( QString, screenName ), QGenericArgument( "QHash< QString, QVariant >", (const void*)&peerdata ) );
}

void
TwitterPlugin::sendOffer( const QString &screenName, const QHash< QString, QVariant > &peerdata )
{
    qDebug() << Q_FUNC_INFO;
    QString offerString = QString( "TOMAHAWKPEER:Host=%1:Port=%2:Node=%3:PKey=%4" ).arg( Servent::instance()->externalAddress() )
                                                                                   .arg( Servent::instance()->externalPort() )
                                                                                   .arg( Database::instance()->dbid() )
                                                                                   .arg( peerdata["okey"].toString() );
    qDebug() << "Sending message to " << screenName << ": " << offerString;
    if( !m_directMessageNew.isNull() )
        m_directMessageNew.data()->post( screenName, offerString );
}

void
TwitterPlugin::directMessageNew(const QTweetDMStatus& message)
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Message sent to " << message.recipientScreenName() << " containing: " << message.text();
}


Q_EXPORT_PLUGIN2( sip, TwitterPlugin )
