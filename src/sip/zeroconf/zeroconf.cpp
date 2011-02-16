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

bool
ZeroconfPlugin::connectPlugin( bool /*startup*/ )
{
    delete m_zeroconf;
    m_zeroconf = new TomahawkZeroconf( Servent::instance()->port(), this );
    QObject::connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                                    SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );

    m_zeroconf->advertise();
    
    m_isOnline = true;
    
    foreach( QStringList *currNode, m_cachedNodes )
    {
        QStringList nodeSet = *currNode;
        if ( !Servent::instance()->connectedToSession( nodeSet[3] ) )
            Servent::instance()->connectToPeer( nodeSet[0], nodeSet[1].toInt(), "whitelist", nodeSet[2], nodeSet[3] );
        delete currNode;
    }
    m_cachedNodes.empty();

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
    qDebug() << "Found LAN host:" << host << port << nodeid;
    
    //FIXME: This doesn't work...why? I never see Found LAN host in debug either, but somehow nodes are being connected...
    if ( !m_isOnline )
    {
        qDebug() << "Not online, so not connecting";
        QStringList *nodeSet = new QStringList();
        *nodeSet << host << QString::number( port ) << name << nodeid;
        m_cachedNodes.insert( nodeSet );
        return;
    }

    if ( !Servent::instance()->connectedToSession( nodeid ) )
        Servent::instance()->connectToPeer( host, port, "whitelist", name, nodeid );
}

Q_EXPORT_PLUGIN2( sip, ZeroconfPlugin )
