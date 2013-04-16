/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef SERVENT_H
#define SERVENT_H

// time before new connection terminates if no auth received
#define AUTH_TIMEOUT 180000

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostInfo>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "Typedefs.h"
#include "Msg.h"

#include <boost/function.hpp>

#include "DllMacro.h"

class Connection;
class Connector;
class ControlConnection;
class StreamConnection;
class ProxyConnection;
class RemoteCollectionConnection;
class PortFwdThread;
class PeerInfo;
class SipInfo;

typedef boost::function< void( const Tomahawk::result_ptr&,
                               boost::function< void( QSharedPointer< QIODevice >& ) > )> IODeviceFactoryFunc;

// this is used to hold a bit of state, so when a connected signal is emitted
// from a socket, we can associate it with a Connection object etc.
class DLLEXPORT QTcpSocketExtra : public QTcpSocket
{
Q_OBJECT

public:
    QTcpSocketExtra() : QTcpSocket()
    {
        QTimer::singleShot( AUTH_TIMEOUT, this, SLOT( authTimeout() ) ) ;
    }

    QPointer<Connection> _conn;
    bool _outbound;
    bool _disowned;
    msg_ptr _msg;

private slots:
    void authTimeout()
    {
      if( _disowned )
          return;

      qDebug() << "Connection timed out before providing a valid offer-key";
      this->disconnectFromHost();
    }
};

class DLLEXPORT Servent : public QTcpServer
{
Q_OBJECT

public:
    static Servent* instance();

    explicit Servent( QObject* parent = 0 );
    virtual ~Servent();

    bool startListening( QHostAddress ha, bool upnp, int port );

    int port() const { return m_port; }

    // creates new token that allows a controlconnection to be set up
    QString createConnectionKey( const QString& name = "", const QString &nodeid = "", const QString &key = "", bool onceOnly = true );

    void registerOffer( const QString& key, Connection* conn );

    void registerControlConnection( ControlConnection* conn );
    void unregisterControlConnection( ControlConnection* conn );
    ControlConnection* lookupControlConnection( const SipInfo& sipInfo );

    // you may call this method as often as you like for the same peerInfo, dupe checking is done inside
    void registerPeer( const Tomahawk::peerinfo_ptr& peerInfo );
    void handleSipInfo( const Tomahawk::peerinfo_ptr& peerInfo );

public slots:
    void onSipInfoChanged();

public:
    void connectToPeer( const Tomahawk::peerinfo_ptr& ha );
    void connectToPeer( const QString& ha, int port, const QString& key, Connection* conn );
    void reverseOfferRequest( ControlConnection* orig_conn, const QString& theirdbid, const QString& key, const QString& theirkey );

    bool visibleExternally() const { return !m_externalHostname.isNull() || (m_externalPort > 0 && !m_externalAddress.isNull()); }
    QString externalAddress() const { return !m_externalHostname.isNull() ? m_externalHostname : m_externalAddress.toString(); }
    int externalPort() const { return m_externalPort; }

    static bool isIPWhitelisted( QHostAddress ip );

    bool connectedToSession( const QString& session );
    unsigned int numConnectedPeers() const { return m_controlconnections.length(); }

    QList< StreamConnection* > streams() const { return m_scsessions; }

    void getIODeviceForUrl( const Tomahawk::result_ptr& result, boost::function< void ( QSharedPointer< QIODevice >& ) > callback );
    void registerIODeviceFactory( const QString &proto, IODeviceFactoryFunc fac );
    void remoteIODeviceFactory( const Tomahawk::result_ptr& result, boost::function< void ( QSharedPointer< QIODevice >& ) > callback );
    void localFileIODeviceFactory( const Tomahawk::result_ptr& result, boost::function< void ( QSharedPointer< QIODevice >& ) > callback );
    void httpIODeviceFactory( const Tomahawk::result_ptr& result, boost::function< void ( QSharedPointer< QIODevice >& ) > callback );

    bool isReady() const { return m_ready; };

signals:
    void dbSyncTriggered();
    void streamStarted( StreamConnection* );
    void streamFinished( StreamConnection* );
    void ready();

protected:
    void incomingConnection( int sd );

public slots:
    void setInternalAddress();
    void setExternalAddress( QHostAddress ha, unsigned int port );

    void socketError( QAbstractSocket::SocketError );
    void createParallelConnection( Connection* orig_conn, Connection* new_conn, const QString& key );

    void registerStreamConnection( StreamConnection* );
    void onStreamFinished( StreamConnection* sc );

    void socketConnected();
    void triggerDBSync();

private slots:
    void readyRead();

    Connection* claimOffer( ControlConnection* cc, const QString &nodeid, const QString &key, const QHostAddress peer = QHostAddress::Any );

private:
    bool isValidExternalIP( const QHostAddress& addr ) const;
    void handoverSocket( Connection* conn, QTcpSocketExtra* sock );
    void printCurrentTransfers();

    QJson::Parser parser;
    QList< ControlConnection* > m_controlconnections; // canonical list of authed peers
    QMap< QString, QPointer< Connection > > m_offers;
    QStringList m_connectedNodes;

    int m_port, m_externalPort;
    QHostAddress m_externalAddress;
    QString m_externalHostname;
    bool m_ready;
    bool m_lanHack;
    bool m_noAuth;

    // currently active file transfers:
    QList< StreamConnection* > m_scsessions;
    QMutex m_ftsession_mut;

    QMap< QString, IODeviceFactoryFunc > m_iofactories;

    QPointer< PortFwdThread > m_portfwd;
    static Servent* s_instance;
};

#endif // SERVENT_H
