/*
    This is the Jabber client that the rest of the app sees
    Gloox stuff should NOT leak outside this class.
    We may replace gloox later, this interface should remain the same.
*/
#ifndef JABBER_P_H
#define JABBER_P_H

#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include <QThread>
#include <QTimer>

#include <string>

#include <gloox/client.h>
#include <gloox/messagesessionhandler.h>
#include <gloox/messagehandler.h>
#include <gloox/messageeventhandler.h>
#include <gloox/messageeventfilter.h>
#include <gloox/chatstatehandler.h>
#include <gloox/chatstatefilter.h>
#include <gloox/connectionlistener.h>
#include <gloox/disco.h>
#include <gloox/message.h>
#include <gloox/discohandler.h>
#include <gloox/stanza.h>
#include <gloox/gloox.h>
#include <gloox/lastactivity.h>
#include <gloox/loghandler.h>
#include <gloox/logsink.h>
#include <gloox/connectiontcpclient.h>
#include <gloox/connectionsocks5proxy.h>
#include <gloox/connectionhttpproxy.h>
#include <gloox/messagehandler.h>
#include <gloox/rostermanager.h>
#include <gloox/siprofileft.h>
#include <gloox/siprofilefthandler.h>
#include <gloox/bytestreamdatahandler.h>
#include <gloox/error.h>
#include <gloox/presence.h>
#include <gloox/rosteritem.h>

#if defined( WIN32 ) || defined( _WIN32 )
# include <windows.h>
#endif

class Jabber_p :
       public QObject,
       public gloox::ConnectionListener,
       public gloox::RosterListener,
       public gloox::MessageHandler,
       gloox::LogHandler
       //public gloox::DiscoHandler,
{
Q_OBJECT

public:
    explicit Jabber_p( const QString& jid, const QString& password, const QString& server = "", const int port = -1 );
    virtual ~Jabber_p();

    void disconnect();

    /// GLOOX IMPLEMENTATION STUFF FOLLOWS
    virtual void onConnect();
    virtual void onDisconnect( gloox::ConnectionError e );
    virtual bool onTLSConnect( const gloox::CertInfo& info );

    virtual void handleMessage( const gloox::Message& msg, gloox::MessageSession * /*session*/ );
    virtual void handleLog( gloox::LogLevel level, gloox::LogArea area, const std::string& message );

    /// ROSTER STUFF
    virtual void onResourceBindError( gloox::ResourceBindError error );
    virtual void onSessionCreateError( gloox::SessionCreateError error );

    virtual void handleItemSubscribed( const gloox::JID& jid );
    virtual void handleItemAdded( const gloox::JID& jid );
    virtual void handleItemUnsubscribed( const gloox::JID& jid );
    virtual void handleItemRemoved( const gloox::JID& jid );
    virtual void handleItemUpdated( const gloox::JID& jid );

    virtual void handleRoster( const gloox::Roster& roster );
    virtual void handleRosterError( const gloox::IQ& /*iq*/ );
    virtual void handleRosterPresence( const gloox::RosterItem& item, const std::string& resource,
                                       gloox::Presence::PresenceType presence, const std::string& /*msg*/ );
    virtual void handleSelfPresence( const gloox::RosterItem& item, const std::string& resource,
                                       gloox::Presence::PresenceType presence, const std::string& msg );
    virtual bool handleSubscriptionRequest( const gloox::JID& jid, const std::string& /*msg*/ );
    virtual bool handleUnsubscriptionRequest( const gloox::JID& jid, const std::string& /*msg*/ );
    virtual void handleNonrosterPresence( const gloox::Presence& presence );
    /// END ROSTER STUFF

    /// DISCO STUFF
    virtual void handleDiscoInfo( const gloox::JID& from, const gloox::Disco::Info& info, int context);
    virtual void handleDiscoItems( const gloox::JID& /*iq*/, const gloox::Disco::Items&, int /*context*/ );
    virtual void handleDiscoError( const gloox::JID& /*iq*/, const gloox::Error*, int /*context*/ );
    /// END DISCO STUFF

protected:
    /////virtual void run();

signals:
    void msgReceived( const QString&, const QString& ); //from, msg
    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void connected();
    void disconnected();
    void jidChanged( const QString& );
    void authError( int, const QString& );

public slots:
    void go();
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString &msg );

private slots:

    void doJabberRecv();

private:
    bool presenceMeansOnline( gloox::Presence::PresenceType p );

    QSharedPointer<gloox::Client> m_client;
    gloox::JID m_jid;
    QMap<gloox::Presence::PresenceType, QString> m_presences;
    QMap<QString, gloox::Presence::PresenceType> m_peers;
    QTimer m_timer; // for recv()
};

#endif // JABBER_H
