#ifndef SERVENT_H
#define SERVENT_H

// port for servent to listen on
#define DEFAULT_LISTEN_PORT 50210
// time before new connection terminates if no auth received
#define AUTH_TIMEOUT 15000

#include <QObject>
#include <QTcpServer>
#include <QHostInfo>
#include <QMap>
#include <QMutex>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QTimer>
#include <QPointer>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "typedefs.h"
#include "msg.h"

#include <boost/function.hpp>

#include "dllmacro.h"

class Connection;
class Connector;
class ControlConnection;
class FileTransferConnection;
class ProxyConnection;
class RemoteCollectionConnection;
class Portfwd;

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

    Connection* _conn;
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

    bool startListening( QHostAddress ha, bool upnp = false, int port = DEFAULT_LISTEN_PORT );

    int port() const { return m_port; }

    // creates new token that allows a controlconnection to be set up
    QString createConnectionKey( const QString& name = "" );

    void registerOffer( const QString& key, Connection* conn );

    void registerControlConnection( ControlConnection* conn );
    void unregisterControlConnection( ControlConnection* conn );
    ControlConnection* lookupControlConnection( const QString& name );

    void connectToPeer( const QString& ha, int port, const QString &key, const QString& name = "", const QString& id = "" );
    void connectToPeer( const QString& ha, int port, const QString &key, Connection* conn );
    void reverseOfferRequest( ControlConnection* orig_conn, const QString& key, const QString& theirkey );

    void setExternalAddress( QHostAddress ha, int port );
    bool visibleExternally() const { return m_externalPort > 0 && !m_externalAddress.isNull(); }
    QHostAddress externalAddress() const { return m_externalAddress; }
    int externalPort() const { return m_externalPort; }

    QSharedPointer<QIODevice> remoteIODeviceFactory( const Tomahawk::result_ptr& );
    static bool isIPWhitelisted( QHostAddress ip );

    bool connectedToSession( const QString& session );
    unsigned int numConnectedPeers() const { return m_controlconnections.length(); }

    QList< FileTransferConnection* > fileTransfers() const { return m_ftsessions; }

    QSharedPointer<QIODevice> getIODeviceForUrl( const Tomahawk::result_ptr& result );
    void registerIODeviceFactory( const QString &proto, boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> fac );
    QSharedPointer<QIODevice> localFileIODeviceFactory( const Tomahawk::result_ptr& result );
    QSharedPointer<QIODevice> httpIODeviceFactory( const Tomahawk::result_ptr& result );

signals:
    void fileTransferStarted( FileTransferConnection* );
    void fileTransferFinished( FileTransferConnection* );

protected:
    void incomingConnection( int sd );

public slots:
    void socketError( QAbstractSocket::SocketError );
    void createParallelConnection( Connection* orig_conn, Connection* new_conn, const QString& key );

    void registerFileTransferConnection( FileTransferConnection* );
    void onFileTransferFinished( FileTransferConnection* ftc );

    void socketConnected();
    void triggerDBSync();

private slots:
    void readyRead();

    Connection* claimOffer( ControlConnection* cc, const QString &key, const QHostAddress peer = QHostAddress::Any );

private:
    void handoverSocket( Connection* conn, QTcpSocketExtra* sock );

    void printCurrentTransfers();

    QJson::Parser parser;
    QList< ControlConnection* > m_controlconnections; // canonical list of authed peers
    QMap< QString, QPointer<Connection> > m_offers;
    int m_port, m_externalPort;
    QHostAddress m_externalAddress;

    // currently active file transfers:
    QList< FileTransferConnection* > m_ftsessions;
    QMutex m_ftsession_mut;

    Portfwd* pf;
    QMap< QString,boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> > m_iofactories;

    static Servent* s_instance;
};

#endif // SERVENT_H
