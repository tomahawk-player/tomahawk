/*
    This is the Jabber client that the rest of the app sees
    Gloox stuff should NOT leak outside this class.
    We may replace jreen later, this interface should remain the same.
*/
#ifndef JABBER_P_H
#define JABBER_P_H

#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include <QNetworkProxy>

#include <string>

#include <jreen/client.h>
#include <jreen/disco.h>
#include <jreen/message.h>
#include <jreen/messagesession.h>
#include <jreen/stanza.h>
#include <jreen/jreen.h>
#include <jreen/error.h>
#include <jreen/presence.h>
#include <jreen/vcard.h>
#include <jreen/abstractroster.h>


#if defined( WIN32 ) || defined( _WIN32 )
# include <windows.h>
#endif

#include "../sipdllmacro.h"
#include <jreen/connection.h>

class SIPDLLEXPORT Jabber_p :
       public QObject
{
Q_OBJECT

public:
    explicit Jabber_p( const QString& jid, const QString& password, const QString& server = "", const int port = -1 );
    virtual ~Jabber_p();

    void setProxy( QNetworkProxy* proxy );

signals:
    void msgReceived( const QString&, const QString& ); //from, msg
    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void connected();
    void disconnected();
    void jidChanged( const QString& );
    void authError( int, const QString& );

public slots:
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString& msg );
    void addContact( const QString& jid, const QString& msg = QString() );
    void disconnect();

    void onDisconnect(jreen::Client::DisconnectReason reason);
    void onConnect();

private slots:
    virtual void onNewPresence( const jreen::Presence& presence );
    virtual void onNewMessage( const jreen::Message& msg );
    virtual void onError( const jreen::Connection::SocketError& e )
    {
        qDebug() << e;
    }

private:
    bool presenceMeansOnline( jreen::Presence::Type p );
    jreen::Client *m_client;
    jreen::JID m_jid;
    QMap<jreen::Presence::Type, QString> m_presences;
    QMap<QString, jreen::Presence::Type> m_peers;
    QString m_server;
};

#endif // JABBER_H
