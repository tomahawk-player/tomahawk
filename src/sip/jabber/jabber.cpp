#include "jabber.h"

#include "tomahawksettings.h"

#include <QtPlugin>
#include <QStringList>


void
JabberPlugin::setProxy( QNetworkProxy* proxy )
{
    p->setProxy( proxy );
}


bool
JabberPlugin::connect( bool startup )
{
    qDebug() << "JabberPlugin::connect";
    if ( startup && !TomahawkSettings::instance()->jabberAutoConnect() )
        return false;

    QString jid       = TomahawkSettings::instance()->jabberUsername();
    QString server    = TomahawkSettings::instance()->jabberServer();
    QString password  = TomahawkSettings::instance()->jabberPassword();
    unsigned int port = TomahawkSettings::instance()->jabberPort();

    QStringList splitJid = jid.split( '@', QString::SkipEmptyParts );
    if ( splitJid.size() < 2 )
    {
        qDebug() << "JID did not have an @ in it, could not find a server part";
        return false;
    }
    
    // gtalk check
    //FIXME: Can remove this once the SRV lookups work
    if ( server.isEmpty() && ( splitJid[1] == "gmail.com" || splitJid[1]  == "googlemail.com" ) )
    {
        qDebug() << "Setting jabber server to talk.google.com";
        server = "talk.google.com";
    }

    if ( server.isEmpty() )
        server = splitJid[1];

    if ( port < 1 || port > 65535 || jid.isEmpty() || password.isEmpty() )
    {
        qDebug() << "Jabber credentials look wrong, not connecting";
        return false;
    }

    delete p;
    p = new Jabber_p( jid, password, server, port );

    QObject::connect( p, SIGNAL( peerOnline( QString ) ), SIGNAL( peerOnline( QString ) ) );
    QObject::connect( p, SIGNAL( peerOffline( QString ) ), SIGNAL( peerOffline( QString ) ) );
    QObject::connect( p, SIGNAL( msgReceived( QString, QString ) ), SIGNAL( msgReceived( QString, QString ) ) );

    QObject::connect( p, SIGNAL( connected() ), SIGNAL( connected() ) );
    QObject::connect( p, SIGNAL( disconnected() ), SIGNAL( disconnected() ) );
    QObject::connect( p, SIGNAL( authError( int, QString ) ), SLOT( onAuthError( int, QString ) ) );

    p->resolveHostSRV();
    
    return true;
}


void
JabberPlugin::onAuthError( int code, const QString& message )
{
    if ( code == gloox::ConnAuthenticationFailed )
        emit error( SipPlugin::AuthError, message );
    else
        emit error( SipPlugin::ConnectionError, message );
}

Q_EXPORT_PLUGIN2( sip, JabberPlugin )
