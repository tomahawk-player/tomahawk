#include "zeroconf.h"

#include <QtPlugin>


bool
ZeroconfPlugin::connect( bool /*startup*/ )
{
    delete m_zeroconf;
    m_zeroconf = new TomahawkZeroconf( Servent::instance()->port(), this );
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                                    SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );

    m_zeroconf->advertise();

    return true;
}


void
ZeroconfPlugin::lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid )
{
    qDebug() << "Found LAN host:" << host << port << nodeid;

    if ( !Servent::instance()->connectedToSession( nodeid ) )
        Servent::instance()->connectToPeer( host, port, "whitelist", name, nodeid );
}

Q_EXPORT_PLUGIN2( sip, ZeroconfPlugin )
