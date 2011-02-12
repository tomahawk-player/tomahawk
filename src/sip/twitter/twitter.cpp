#include "twitter.h"

#include <QtPlugin>
#include <QRegExp>

#include <qtweetaccountverifycredentials.h>
#include <qtweetuser.h>
#include <qtweetfriendstimeline.h>
#include <qtweetstatus.h>

#include <utils/tomahawkutils.h>
#include <tomahawksettings.h>


TwitterPlugin::TwitterPlugin()
    : SipPlugin()
    , m_isValid( false )
    , m_checkTimer( this )
{
    m_checkTimer.setInterval( 60000 );
    m_checkTimer.setSingleShot( false );
    QObject::connect( &m_checkTimer, SIGNAL( timeout() ), SLOT( checkTimerFired() ) );
    m_checkTimer.start();
}

bool
TwitterPlugin::isValid()
{
    return m_isValid;
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
    qDebug() << "TwitterPlugin connectPlugin called";
    
    TomahawkSettings *settings = TomahawkSettings::instance();
    
    if ( settings->twitterOAuthToken().isEmpty() || settings->twitterOAuthTokenSecret().isEmpty() )
    {
        qDebug() << "Empty Twitter credentials; not connecting";
        return false;
    }
 
    delete m_twitterAuth.data();
    m_twitterAuth = QWeakPointer<TomahawkOAuthTwitter>( new TomahawkOAuthTwitter( this ) );
    m_twitterAuth.data()->setNetworkAccessManager( TomahawkUtils::nam() );
    m_twitterAuth.data()->setOAuthToken( settings->twitterOAuthToken().toLatin1() );
    m_twitterAuth.data()->setOAuthTokenSecret( settings->twitterOAuthTokenSecret().toLatin1() );
    
    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
    QObject::connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( connectAuthVerifyReply(const QTweetUser &) ) );
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
    if( !m_friendsTimeline.isNull() )
        m_friendsTimeline.data()->deleteLater();
    if( !m_twitterAuth.isNull() )
        m_twitterAuth.data()->deleteLater();
}

void
TwitterPlugin::connectAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        qDebug() << "Could not authenticate to Twitter";
        m_isValid = false;
    }
    else
    {
        qDebug() << "TwitterPlugin successfully authenticated to Twitter as user " << user.screenName();
        m_isValid = true;
        if ( !m_twitterAuth.isNull() )
        {
            m_friendsTimeline = QWeakPointer<QTweetFriendsTimeline>( new QTweetFriendsTimeline( m_twitterAuth.data(), this ) );
            QObject::connect( m_friendsTimeline.data(), SIGNAL( parsedStatuses(const QList< QTweetStatus > &) ), SLOT( friendsTimelineStatuses(const QList<QTweetStatus> &) ) );
            QMetaObject::invokeMethod( this, "checkTimerFired", Qt::DirectConnection );
        }
        else
        {
            qDebug() << "Twitter auth pointer was null!";
            m_isValid = false;
        }
    }
}

void
TwitterPlugin::checkTimerFired()
{
    if ( isValid() )
    {
        qint64 cachedfriendssinceid = TomahawkSettings::instance()->twitterCachedFriendsSinceId();
        if ( !m_friendsTimeline.isNull() )
            m_friendsTimeline.data()->fetch( cachedfriendssinceid, 0, 800 );
    }
}

void
TwitterPlugin::friendsTimelineStatuses( const QList< QTweetStatus >& statuses )
{
    QRegExp regex( QString( "^(@[a-zA-Z0-9]+ )?Got Tomahawk\\?(.*)$" ) );
    foreach( QTweetStatus status, statuses )
    {
        QString statusText = status.text();
        qDebug() << "Performing matching on status text " << statusText;
        if ( regex.exactMatch( statusText ) )
        {
            qDebug() << "Found an exact matching Tweet from user " << status.user().screenName();
        }
        else
            qDebug() << "No match, matched length is " << regex.matchedLength();
    }
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
