#ifndef JABBER_H
#define JABBER_H

#include "SipPlugin.h"
#include "jabber_p.h"

class JabberPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )

public:
    JabberPlugin()
        : p( 0 )
    {}

    virtual ~JabberPlugin() { delete p; }

    void setProxy( QNetworkProxy* proxy );

public slots:
    virtual bool connect();

    void disconnect()
    {
        QMetaObject::invokeMethod( p,
                                   "disconnect",
                                   Qt::QueuedConnection
                                 );
    }

    void sendMsg( const QString& to, const QString& msg )
    {
        QMetaObject::invokeMethod( p,
                                   "sendMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, to),
                                   Q_ARG(const QString, msg)
                                 );
    }

    void broadcastMsg( const QString &msg )
    {
        QMetaObject::invokeMethod( p,
                                   "broadcastMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, msg)
                                 );
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
        QMetaObject::invokeMethod( p,
                                   "addContact",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, jid),
                                   Q_ARG(const QString, msg)
                                 );
    }

private slots:
    void onAuthError( int, const QString& );

private:
    Jabber_p* p;
};

#endif
