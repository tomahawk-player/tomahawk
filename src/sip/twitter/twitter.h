#ifndef TWITTER_H
#define TWITTER_H

#include <QTimer>
#include <QWeakPointer>

#include <qtweetuser.h>
#include <qtweetnetbase.h>
#include <qtweetfriendstimeline.h>
#include <qtweetmentions.h>

#include "../sipdllmacro.h"
#include "sip/SipPlugin.h"
#include "tomahawkoauthtwitter.h"

#define MYNAME "SIPTWITTER"

class SIPDLLEXPORT TwitterPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )
    
public:
    TwitterPlugin();

    virtual ~TwitterPlugin() {}
    
    virtual bool isValid();
    virtual const QString name();

public slots:
    virtual bool connectPlugin( bool startup );

    void disconnectPlugin();

    void sendMsg( const QString& to, const QString& msg )
    {
    }

    void broadcastMsg( const QString &msg )
    {
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
    }

private slots:
    void lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid );
    void connectAuthVerifyReply( const QTweetUser &user );
    void checkTimerFired();
    void friendsTimelineStatuses( const QList< QTweetStatus > &statuses );
    void mentionsStatuses( const QList< QTweetStatus > &statuses );

private:
    QWeakPointer<TomahawkOAuthTwitter> m_twitterAuth;
    QWeakPointer<QTweetFriendsTimeline> m_friendsTimeline;
    QWeakPointer<QTweetMentions> m_mentions;
    bool m_isValid;
    QTimer m_checkTimer;
    qint64 m_cachedFriendsSinceId;
    qint64 m_cachedMentionsSinceId;
};

#endif
