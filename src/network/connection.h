#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSharedPointer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QVariant>
#include <QVariantMap>
#include <QString>
#include <QDebug>
#include <QDataStream>
#include <QtEndian>
#include <QTimer>
#include <QTime>
#include <QPointer>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "msg.h"
#include "msgprocessor.h"

class Servent;

class Connection : public QObject
{
Q_OBJECT

public:

    Connection( Servent* parent );
    virtual ~Connection();
    virtual Connection* clone() = 0;

    QString id() const;
    void setId( const QString& );

    void setFirstMessage( const QVariant& m );
    void setFirstMessage( msg_ptr m );
    msg_ptr firstMessage() const { return m_firstmsg; };

    const QPointer<QTcpSocket>& socket() { return m_sock; };

    void setOutbound( bool o ) { m_outbound = o; };
    bool outbound() const { return m_outbound; }

    Servent* servent() { return m_servent; };

    // get public port of remote peer:
    int peerPort() { return m_peerport; };
    void setPeerPort( int p ) { m_peerport = p; };

    void markAsFailed();

    void setName( const QString& n ) { m_name = n; };
    QString name() const { return m_name; };

    void setOnceOnly( bool b ) { m_onceonly = b; };
    bool onceOnly() const { return m_onceonly; };

    bool isReady() const { return m_ready; } ;
    bool isRunning() const { return m_sock != 0; }

    qint64 bytesSent() const { return m_tx_bytes; }
    qint64 bytesReceived() const { return m_rx_bytes; }

    void setMsgProcessorModeOut( quint32 m ) { m_msgprocessor_out.setMode( m ); }
    void setMsgProcessorModeIn( quint32 m ) { m_msgprocessor_in.setMode( m ); }

signals:
    void ready();
    void failed();
    void finished();
    void statsTick( qint64 tx_bytes_sec, qint64 rx_bytes_sec );
    void socketClosed();
    void socketErrored( QAbstractSocket::SocketError );

protected:
    virtual void setup() = 0;

protected slots:
    virtual void handleMsg( msg_ptr msg ) = 0;

public slots:
    virtual void start( QTcpSocket* sock );
    void sendMsg( QVariant );
    void sendMsg( msg_ptr );

    void shutdown( bool waitUntilSentAll = false );

private slots:
    void handleIncomingQueueEmpty();
    void sendMsg_now( msg_ptr );
    void socketDisconnected();
    void socketDisconnectedError( QAbstractSocket::SocketError );
    void readyRead();
    void doSetup();
    void authCheckTimeout();
    void bytesWritten( qint64 );
    void calcStats();

protected:
    QPointer<QTcpSocket> m_sock;
    int m_peerport;
    msg_ptr m_msg;
    QJson::Parser parser;
    Servent* m_servent;
    bool m_outbound, m_ready, m_onceonly;
    msg_ptr m_firstmsg;
    QString m_name;

private:
    void handleReadMsg();
    void actualShutdown();
    bool m_do_shutdown, m_actually_shutting_down, m_peer_disconnected;
    qint64 m_tx_bytes, m_tx_bytes_requested;
    qint64 m_rx_bytes;
    QString m_id;

    QTimer* m_statstimer;
    QTime m_statstimer_mark;
    qint64 m_stats_tx_bytes_per_sec, m_stats_rx_bytes_per_sec;
    qint64 m_rx_bytes_last, m_tx_bytes_last;

    MsgProcessor m_msgprocessor_in, m_msgprocessor_out;
};

#endif // CONNECTION_H
