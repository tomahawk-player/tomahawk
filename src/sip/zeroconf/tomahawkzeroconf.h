#ifndef TOMAHAWKZCONF
#define TOMAHAWKZCONF

#define ZCONF_PORT 50210

#include <QDebug>
#include <QList>
#include <QHostAddress>
#include <QHostInfo>
#include <QUdpSocket>
#include <QTimer>

#include "database/database.h"
#include "network/servent.h"

#include "dllmacro.h"

class DLLEXPORT Node : public QObject
{
Q_OBJECT

public:
    Node( const QString& i, const QString& n, int p )
        : ip( i ), nid( n ), port( p )
    {
        qDebug() << Q_FUNC_INFO;
    }

    ~Node()
    {
        qDebug() << Q_FUNC_INFO;
    }

signals:
    void tomahawkHostFound( const QString&, int, const QString&, const QString& );

public slots:
    void resolved( QHostInfo i )
    {
        emit tomahawkHostFound( ip, port, i.hostName(), nid );
        this->deleteLater();
    }

    void resolve()
    {
        QHostInfo::lookupHost( ip, this, SLOT( resolved( QHostInfo ) ) );
    }

private:
    QString ip;
    QString nid;
    int port;
};


class DLLEXPORT TomahawkZeroconf : public QObject
{
Q_OBJECT

public:
    TomahawkZeroconf( int port, QObject* parent = 0 )
        : QObject( parent ), m_sock( this ), m_port( port )
    {
        m_sock.bind( ZCONF_PORT, QUdpSocket::ShareAddress );
        connect( &m_sock, SIGNAL( readyRead() ), this, SLOT( readPacket() ) );
    }

public slots:
    void advertise()
    {
        qDebug() << "Advertising us on the LAN";
        QByteArray advert = QString( "TOMAHAWKADVERT:%1:%2" )
                            .arg( m_port )
                            .arg( Database::instance()->dbid() )
                            .toAscii();
        m_sock.writeDatagram( advert.data(), advert.size(),
                              QHostAddress::Broadcast, ZCONF_PORT );
    }

signals:
    // IP, port, name, session
    void tomahawkHostFound( const QString&, int, const QString&, const QString& );

private slots:
    void readPacket()
    {
        if ( !m_sock.hasPendingDatagrams() )
            return;

        QByteArray datagram;
        datagram.resize( m_sock.pendingDatagramSize() );
        QHostAddress sender;
        quint16 senderPort;
        m_sock.readDatagram( datagram.data(), datagram.size(), &sender, &senderPort );
        qDebug() << "DATAGRAM RCVD" << QString::fromAscii( datagram ) << sender;

        // only process msgs originating on the LAN:
        if ( datagram.startsWith( "TOMAHAWKADVERT:" ) &&
            Servent::isIPWhitelisted( sender ) )
        {
            QStringList parts = QString::fromAscii( datagram ).split( ':' );
            if ( parts.length() == 3 )
            {
                bool ok;
                int port = parts.at(1).toInt( &ok );
                if(ok && Database::instance()->dbid() != parts.at( 2 ) )
                {
                    qDebug() << "ADVERT received:" << sender << port;
                    Node *n = new Node( sender.toString(), parts.at( 2 ), port );
                    connect( n,    SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                             this, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ) );
                    n->resolve();
                }
            }
        }

        if ( m_sock.hasPendingDatagrams() )
            QTimer::singleShot( 0, this, SLOT( readPacket() ) );
    }

private:
    QUdpSocket m_sock;
    int m_port;
};

#endif

