/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOMAHAWKZCONF
#define TOMAHAWKZCONF

#define ZCONF_PORT 50210

#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "network/Servent.h"
#include "accounts/AccountDllMacro.h"

#include <QList>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkProxy>
#include <QUdpSocket>
#include <QTimer>

class Node : public QObject
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
        qDebug() << Q_FUNC_INFO << "zeroconf-derived IP has resolved to name " << i.hostName();
        if ( i.hostName().length() )
            emit tomahawkHostFound( ip, port, i.hostName(), nid );
        else
            emit tomahawkHostFound( ip, port, "Unknown", nid );
        this->deleteLater();
    }

    void resolve()
    {
        qDebug() << Q_FUNC_INFO << "Resolving zeroconf-derived IP " << ip;
        QHostInfo::lookupHost( ip, this, SLOT( resolved( QHostInfo ) ) );
    }

private:
    QString ip;
    QString nid;
    int port;
};


class ACCOUNTDLLEXPORT TomahawkZeroconf : public QObject
{
Q_OBJECT

public:
    TomahawkZeroconf( int port, QObject* parent = 0 )
        : QObject( parent ), m_sock( this ), m_port( port )
    {
        qDebug() << Q_FUNC_INFO;
        m_sock.setProxy( QNetworkProxy::NoProxy );
        m_sock.bind( ZCONF_PORT, QUdpSocket::ShareAddress );
        connect( &m_sock, SIGNAL( readyRead() ), this, SLOT( readPacket() ) );
    }

    virtual ~TomahawkZeroconf()
    {
        qDebug() << Q_FUNC_INFO;
    }

public slots:
    void advertise()
    {
        qDebug() << "Advertising us on the LAN (both versions)";
        // Keep newer versions first
        QByteArray advert = QString( "TOMAHAWKADVERT:%1:%2:%3" )
                               .arg( m_port )
                               .arg( Database::instance()->impl()->dbid() )
                               .arg( QHostInfo::localHostName() )
                               .toLatin1();
        m_sock.writeDatagram( advert.data(), advert.size(), QHostAddress::Broadcast, ZCONF_PORT );

        advert = QString( "TOMAHAWKADVERT:%1:%2" )
                    .arg( m_port )
                    .arg( Database::instance()->impl()->dbid() )
                    .toLatin1();
        m_sock.writeDatagram( advert.data(), advert.size(), QHostAddress::Broadcast, ZCONF_PORT );
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
        qDebug() << "DATAGRAM RCVD" << QString::fromLatin1( datagram ) << sender;

        // only process msgs originating on the LAN:
        if ( datagram.startsWith( "TOMAHAWKADVERT:" ) &&
            Servent::isIPWhitelisted( sender ) )
        {
            QStringList parts = QString::fromLatin1( datagram ).split( ':' );
            if ( parts.length() == 4 )
            {
                bool ok;
                int port = parts.at(1).toInt( &ok );
                if ( ok && Database::instance()->impl()->dbid() != parts.at( 2 ) )
                {
                    emit tomahawkHostFound( sender.toString(), port, parts.at( 3 ), parts.at( 2 ) );
                }
            }
            else if ( parts.length() == 3 )
            {
                bool ok;
                int port = parts.at(1).toInt( &ok );
                if ( ok && Database::instance()->impl()->dbid() != parts.at( 2 ) )
                {
                    qDebug() << "ADVERT received:" << sender << port;
                    Node *n = new Node( sender.toString(), parts.at( 2 ), port );
                    connect( n,    SIGNAL( tomahawkHostFound( QString, int, QString, QString ) ),
                             this, SIGNAL( tomahawkHostFound( QString, int, QString, QString ) ) );
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

