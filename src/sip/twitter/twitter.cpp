#include "twitter.h"

#include <QtPlugin>
#include <QRegExp>

#include <qtweetaccountverifycredentials.h>
#include <qtweetuser.h>
#include <qtweetstatus.h>

#include <utils/tomahawkutils.h>
#include <tomahawksettings.h>


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
    foreach( QString key, peerlist )
    {
        qDebug() << "TwitterPlugin found cached peer with host " << m_cachedPeers[key].toHash()["host"].toString() << " and port " << m_cachedPeers[key].toHash()["port"].toString();
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
    /*
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                                    SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );
    */
    
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
            connect( m_friendsTimeline.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( friendsTimelineStatuses(const QList<QTweetStatus> &) ) );
            connect( m_mentions.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( mentionsStatuses(const QList<QTweetStatus> &) ) );
            connect( m_directMessages.data(), SIGNAL( parsedStatuses(const QList< QTweetDMStatus > &) ), SLOT( mentionsStatuses(const QList<QTweetDMStatus> &) ) );
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
    foreach( QTweetStatus status, statuses )
    {
        if ( status.id() > m_cachedFriendsSinceId )
            m_cachedFriendsSinceId = status.id();
        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact tweet from friend " << status.user().screenName();
            QHash<QString, QVariant> peerData;
            if ( ! m_cachedPeers.contains( status.user().screenName() ) )
            {
                QHash< QString, QVariant > peerData;
                peerData["sentuid"] = QVariant::fromValue< QString >( uuid() );
                m_cachedPeers[status.user().screenName()] = QVariant::fromValue< QHash< QString, QVariant > >( peerData );
            }
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
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?Got Tomahawk\\?(.*)$" ) );
    foreach( QTweetStatus status, statuses )
    {
        if ( status.id() > m_cachedMentionsSinceId )
            m_cachedMentionsSinceId = status.id();
        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact matching mention from user " << status.user().screenName();
            if ( ! m_cachedPeers.contains( status.user().screenName() ) )
            {
                QHash< QString, QVariant > peerData;
                peerData["sentuid"] = QVariant::fromValue< QString >( uuid() );
                m_cachedPeers[status.user().screenName()] = QVariant::fromValue< QHash< QString, QVariant > >( peerData );
            }
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
    qDebug() << "TwitterPlugin using direct messages id of " << m_cachedDirectMessagesSinceId;
    if ( !m_directMessages.isNull() )
        m_directMessages.data()->fetch( m_cachedDirectMessagesSinceId, 0, 800 );
}

void
TwitterPlugin::directMessages( const QList< QTweetDMStatus > &messages )
{
    qDebug() << Q_FUNC_INFO;
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?Got Tomahawk\\?(.*)$" ) );
    foreach( QTweetDMStatus status, messages )
    {
        if ( status.id() > m_cachedDirectMessagesSinceId )
            m_cachedDirectMessagesSinceId = status.id();
        if ( regex.exactMatch( status.text() ) )
        {
            qDebug() << "TwitterPlugin found an exact matching mention from user " << status.sender().screenName();
            if ( ! m_cachedPeers.contains( status.sender().screenName() ) )
            {
                QHash< QString, QVariant > peerData;
                peerData["sentuid"] = QVariant::fromValue< QString >( uuid() );
                m_cachedPeers[status.sender().screenName()] = QVariant::fromValue< QHash< QString, QVariant > >( peerData );
            }
        }
    }
    
    TomahawkSettings::instance()->setTwitterCachedDirectMessagesSinceId( m_cachedDirectMessagesSinceId );
    
}

void
TwitterPlugin::lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid )
{
    /*
    qDebug() << "Found LAN host:" << host << port << nodeid;

    if ( !Servent::instance()->connectedToSession( nodeid ) )
        Servent::instance()->connectToPeer( host, port, "whitelist", name, nodeid );
    */
}

Q_EXPORT_PLUGIN2( sip, TwitterPlugin )
