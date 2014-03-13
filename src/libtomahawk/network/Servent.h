/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

// time before new connection terminate if it could not be established
#define CONNECT_TIMEOUT 10000

#include "network/Enums.h"

#include "DllMacro.h"
#include "Typedefs.h"

#include <QHostAddress>
#include <QTcpServer>

class Connection;
class Connector;
class ControlConnection;
class NetworkReply;
class PeerInfo;
class PortFwdThread;
class ProxyConnection;
class QTcpSocketExtra;
class RemoteCollectionConnection;
class SipInfo;
class StreamConnection;

namespace boost
{
    template <class T> class function;
} // boost

class ServentPrivate;

class DLLEXPORT Servent : public QTcpServer
{
Q_OBJECT

public:
    static Servent* instance();
    static bool isValidExternalIP( const QHostAddress& addr );
    static SipInfo getSipInfoForOldVersions( const QList<SipInfo> &sipInfos );

    explicit Servent( QObject* parent = 0 );
    virtual ~Servent();

    bool startListening( QHostAddress ha, bool upnp, int port, Tomahawk::Network::ExternalAddress::Mode mode, int defaultPort, bool autoDetectExternalIp = false, const QString& externalHost = "", int externalPort = -1 );

    // creates new token that allows a controlconnection to be set up
    QString createConnectionKey( const QString& name = "", const QString &nodeid = "", const QString &key = "", bool onceOnly = true );

    void registerOffer( const QString& key, Connection* conn );
    void registerLazyOffer( const QString& key, const Tomahawk::peerinfo_ptr& peerInfo, const QString &nodeid , const int timeout );

    void registerControlConnection( ControlConnection* conn );
    void unregisterControlConnection( ControlConnection* conn );
    ControlConnection* lookupControlConnection( const SipInfo& sipInfo );
    ControlConnection* lookupControlConnection( const QString& nodeid );

    void remoteIODeviceFactory( const Tomahawk::result_ptr& result, const QString& url,
                                    boost::function< void ( QSharedPointer< QIODevice >& ) > callback );

    // you may call this method as often as you like for the same peerInfo, dupe checking is done inside
    void registerPeer( const Tomahawk::peerinfo_ptr& peerInfo );
    void handleSipInfo( const Tomahawk::peerinfo_ptr& peerInfo );

    void initiateConnection( const SipInfo& sipInfo, Connection* conn );
    void reverseOfferRequest( ControlConnection* orig_conn, const QString &theirdbid, const QString& key, const QString& theirkey );

    bool visibleExternally() const;

    /**
     * Is the probality that this host supports IPv6 high?
     *
     * Though we cannot fully test for IPv6 connectivity, some guesses based on non-localhost addresses are done.
     */
    bool ipv6ConnectivityLikely() const;

    /**
     * The port this Peer listens directly (per default)
     */
    int port() const;

    /**
     * The IP addresses this Peer listens directly (per default)
     */
    QList< QHostAddress > addresses() const;

    /**
     * An additional address this peer listens to, e.g. via UPnP.
     */
    QString additionalAddress() const;

    /**
     * An additional port this peer listens to, e.g. via UPnP (only in combination with additionalAddress.
     */
    int additionalPort() const;

    static bool isIPWhitelisted( QHostAddress ip );

    bool connectedToSession( const QString& session );
    unsigned int numConnectedPeers() const;

    QList< StreamConnection* > streams() const;

    bool isReady() const;

    QList<SipInfo> getLocalSipInfos(const QString& nodeid, const QString &key);

    void queueForAclResult( const QString& username, const QSet<Tomahawk::peerinfo_ptr>& peerInfos );
signals:
    void dbSyncTriggered();

    /**
     * @brief ipDetectionFailed Emitted when the automatic external IP detection failed.
     * @param error If the failure was caused by a network error, this is its error code.
     *              If the error wasn't network related, QNetworkReply::NoError will be returned.
     * @param errorString A string explaining the error.
     */
    void ipDetectionFailed( QNetworkReply::NetworkError error, QString errorString );
    void streamStarted( StreamConnection* );
    void streamFinished( StreamConnection* );
    void ready();

protected:
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    virtual void incomingConnection( qintptr sd );
#else
    virtual void incomingConnection( int sd );
#endif

public slots:
    void setExternalAddress( QHostAddress ha, unsigned int port );

    void createParallelConnection( Connection* orig_conn, Connection* new_conn, const QString& key );

    void registerStreamConnection( StreamConnection* );
    void onStreamFinished( StreamConnection* sc );

    void socketConnected();
    void triggerDBSync();

    void onSipInfoChanged();

private slots:
    void deleteLazyOffer( const QString& key );
    void readyRead();
    void socketError( QAbstractSocket::SocketError e );
    void checkACLResult( const QString &nodeid, const QString &username, Tomahawk::ACLStatus::Type peerStatus );
    void ipDetected();

    Connection* claimOffer( ControlConnection* cc, const QString &nodeid, const QString &key, const QHostAddress peer = QHostAddress::Any );

private:
    Q_DECLARE_PRIVATE( Servent )
    ServentPrivate* d_ptr;

    void handoverSocket( Connection* conn, QTcpSocketExtra* sock );
    void cleanupSocket( QTcpSocketExtra* sock );
    void printCurrentTransfers();

    static Servent* s_instance;
};

#endif // SERVENT_H
