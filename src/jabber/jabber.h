#ifndef JABBER_H
#define JABBER_H
/*
    Pimpl of jabber_p, which inherits from a gazillion gloox classes
    and it littered with public methods.
 */
#include "jabber_p.h"

class Jabber : public QObject
{
    Q_OBJECT
public:

    Jabber( const QString &jid, const QString password, const QString server = "", const int port=-1 )
       : p( jid, password, server, port )
    {
    }

    ~Jabber()
    {
       // p.disconnect();
    }

    void setProxy( QNetworkProxy* proxy )
    {
        p.setProxy( proxy );
    }

public slots:

    void start()
    {
        //connect( &p,    SIGNAL(finished()),
        //         this,  SIGNAL(finished()) );

        connect( &p,    SIGNAL(msgReceived(QString,QString)),
                 this,  SIGNAL(msgReceived(QString,QString)) );

        connect( &p,    SIGNAL(peerOnline(QString)),
                 this,  SIGNAL(peerOnline(QString)) );

        connect( &p,    SIGNAL(peerOffline(QString)),
                 this,  SIGNAL(peerOffline(QString)) );

        connect( &p,    SIGNAL(connected()),
                 this,  SIGNAL(connected()) );

        connect( &p,    SIGNAL(disconnected()),
                 this,  SIGNAL(disconnected()) );

        connect( &p,    SIGNAL(jidChanged(QString)),
                 this,  SIGNAL(jidChanged(QString)) );

        connect( &p,    SIGNAL(authError(int,const QString&)),
                 this,  SIGNAL(authError(int,const QString&)) );

        p.go();
    }

    void disconnect()
    {
        QMetaObject::invokeMethod( &p,
                                   "disconnect",
                                   Qt::QueuedConnection
                                 );
    }

    void sendMsg(const QString& to, const QString& msg)
    {
        QMetaObject::invokeMethod( &p,
                                   "sendMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, to),
                                   Q_ARG(const QString, msg)
                                 );
    }

    void broadcastMsg(const QString &msg)
    {
        QMetaObject::invokeMethod( &p,
                                   "broadcastMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, msg)
                                 );
    }

signals:
    //void finished();

    void msgReceived(const QString&, const QString&); //from, msg
    void peerOnline(const QString&);
    void peerOffline(const QString&);
    void connected();
    void disconnected();
    void jidChanged(const QString&);
    void authError(int, const QString&);

private:
    Jabber_p p;
};

#endif
