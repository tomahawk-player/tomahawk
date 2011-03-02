#include "zeroconf.h"

#include <QtPlugin>

const QString
ZeroconfPlugin::name()
{
    return QString( MYNAME );
}

const QString
ZeroconfPlugin::accountName()
{
    return QString();
}

const QString
ZeroconfPlugin::friendlyName()
{
    return QString( "Zeroconf" );
}

bool
ZeroconfPlugin::connectPlugin( bool /*startup*/ )
{
    delete m_zeroconf;
    m_zeroconf = new TomahawkZeroconf( Servent::instance()->port(), this );
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                                    SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );

    m_zeroconf->advertise();
    m_isOnline = true;
    
    return true;
}

void
ZeroconfPlugin::disconnectPlugin()
{
    m_isOnline = false;

    

    delete m_zeroconf;
    m_zeroconf = 0;
}

void
ZeroconfPlugin::lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid )
{
    if ( sender() != m_zeroconf )
        return;

    qDebug() << "Found LAN host:" << host << port << nodeid;
    
    if ( !Servent::instance()->connectedToSession( nodeid ) )
        Servent::instance()->connectToPeer( host, port, "whitelist", name, nodeid );
    else
        qDebug() << "Already connected to" << host;
}

Q_EXPORT_PLUGIN2( sip, ZeroconfPlugin )
