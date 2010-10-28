#ifndef XMPPBOT_H
#define XMPPBOT_H

#include <tomahawk/result.h>
#include <tomahawk/infosystem.h>

#include <QtCore/QObject>
#include <QtCore/qsharedpointer.h>
#include <QTimer>

#include <gloox/messagehandler.h>
#include <gloox/messagesessionhandler.h>
#include <gloox/client.h>
#include <gloox/connectionlistener.h>
#include <gloox/subscriptionhandler.h>
#include <gloox/messagehandler.h>

class XMPPBotClient
    : public QObject
    , public gloox::Client
{
    Q_OBJECT

public:
    XMPPBotClient(QObject* parent, gloox::JID &jid, std::string password, int port);
    virtual ~XMPPBotClient();
    
    void run();
    
private slots:
    void recvSlot();
    
private:
    QTimer m_timer;
};

class XMPPBot
    : public QObject
    , public gloox::ConnectionListener
    , public gloox::SubscriptionHandler
    , public gloox::MessageHandler
{
    Q_OBJECT
    
public:
    XMPPBot(QObject *parent);
    virtual ~XMPPBot();

public slots:
    virtual void newTrackSlot(const Tomahawk::result_ptr &track);
    virtual void infoReturnedSlot(QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData);
    virtual void infoFinishedSlot(QString caller);
    
protected:
    // ConnectionListener
    virtual void onConnect();
    virtual void onDisconnect(gloox::ConnectionError e);
    virtual bool onTLSConnect(const gloox::CertInfo &info);
    
    // SubscriptionHandler
    virtual void handleSubscription(const gloox::Subscription &subscription);
    
    // MessageHandler
    virtual void handleMessage(const gloox::Message &msg, gloox::MessageSession *session = 0);

private slots:
    void onResultsAdded( const QList<Tomahawk::result_ptr>& result );

private:
    QWeakPointer<XMPPBotClient> m_client;
    Tomahawk::result_ptr m_currTrack;
    Tomahawk::InfoSystem::InfoMap m_currInfoMap;
    QString m_currReturnMessage;
    QString m_currReturnJid;
};

#endif // XMPPBOT_H
