#include "twitter.h"

#include <tomahawksettings.h>

#include <QtPlugin>

bool
TwitterPlugin::connect( bool /*startup*/ )
{
    delete m_twitterAuth;
    m_twitterAuth = new OAuthTwitter( this );
    
    TomahawkSettings *settings = TomahawkSettings::instance();
    QString oauthtoken = settings->twitterOAuthToken();
    QString oauthtokensecret = settings->twitterOAuthTokenSecret();
    
    if ( oauthtoken.isEmpty() || oauthtokensecret.isEmpty() )
    {
        qDebug() << "Empty Twitter credentials; not connecting";
        return false;
    }
    
    m_twitterAuth->setOAuthToken( oauthtoken );
    m_twitterAuth->setOAuthTokenSecret( oauthtokensecret );
    
    /*
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                                    SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );
    */
    
    return true;
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
