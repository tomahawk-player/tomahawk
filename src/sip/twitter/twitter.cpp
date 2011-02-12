#include "twitter.h"

#include <tomahawksettings.h>

#include <qtweetaccountverifycredentials.h>
#include <qtweetuser.h>

#include <QtPlugin>
#include <utils/tomahawkutils.h>

TwitterPlugin::TwitterPlugin()
    : m_twitterAuth( 0 )
    , m_isAuthed( false )
{
}

bool
TwitterPlugin::isValid()
{
    return m_isAuthed;
}

bool
TwitterPlugin::connect( bool /*startup*/ )
{    
    TomahawkSettings *settings = TomahawkSettings::instance();
    
    if ( settings->twitterOAuthToken().isEmpty() || settings->twitterOAuthTokenSecret().isEmpty() )
    {
        qDebug() << "Empty Twitter credentials; not connecting";
        return false;
    }
 
    delete m_twitterAuth;
    m_twitterAuth = new TomahawkOAuthTwitter( this );
    m_twitterAuth->setNetworkAccessManager( TomahawkUtils::nam() );
    m_twitterAuth->setOAuthToken( settings->twitterOAuthToken().toLatin1() );
    m_twitterAuth->setOAuthTokenSecret( settings->twitterOAuthTokenSecret().toLatin1() );
    
    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth, this );
    QObject::connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( connectAuthVerifyReply(const QTweetUser &) ) );
    credVerifier->verify();
    /*
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                                    SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );
    */
    
    return true;
}

void
TwitterPlugin::connectAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        qDebug() << "Could not authenticate to Twitter";
        m_isAuthed = false;
    }
    else
    {
        qDebug() << "Successfully authenticated to Twitter as user " << user.screenName();
        m_isAuthed = true;
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
