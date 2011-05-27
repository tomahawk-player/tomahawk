/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TWITTER_H
#define TWITTER_H

#include "twitterconfigwidget.h"

#include <QTimer>
#include <QWeakPointer>
#include <QSet>

#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetnetbase.h>
#include <QTweetLib/qtweetfriendstimeline.h>
#include <QTweetLib/qtweetdirectmessages.h>
#include <QTweetLib/qtweetdirectmessagenew.h>
#include <QTweetLib/qtweetdirectmessagedestroy.h>
#include <QTweetLib/qtweetmentions.h>
#include <QTweetLib/qtweetdmstatus.h>

#include "../sipdllmacro.h"
#include "sip/SipPlugin.h"
#include "tomahawkoauthtwitter.h"

#define MYNAME "SIPTWITTER"

class SIPDLLEXPORT TwitterFactory : public SipPluginFactory
{
    Q_OBJECT
    Q_INTERFACES( SipPluginFactory )

public:
    TwitterFactory() {}
    virtual ~TwitterFactory() {}

    virtual QString prettyName() const { return "Twitter"; }
    virtual QString factoryId() const { return "siptwitter"; }
    virtual QIcon icon() const;
    virtual SipPlugin* createPlugin( const QString& pluginId = QString() );
};

class SIPDLLEXPORT TwitterPlugin : public SipPlugin
{
    Q_OBJECT

public:
    TwitterPlugin( const QString& pluginId );

    virtual ~TwitterPlugin() {}

    virtual bool isValid() const;
    virtual const QString name() const;
    virtual const QString accountName() const;
    virtual const QString friendlyName() const;
    virtual ConnectionState connectionState() const;
    virtual QIcon icon() const;
    virtual QWidget* configWidget();

signals:
    void avatarReceived( QString, QPixmap );
    
public slots:
    virtual bool connectPlugin( bool startup );
    void disconnectPlugin();
    void checkSettings();
    void refreshProxy();

    void sendMsg( const QString& to, const QString& msg )
    {
        Q_UNUSED( to );
        Q_UNUSED( msg );
    }

    void broadcastMsg( const QString &msg )
    {
        Q_UNUSED( msg );
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
        Q_UNUSED( jid );
        Q_UNUSED( msg );
    }

private slots:
    void configDialogAuthedSignalSlot( bool authed );
    void connectAuthVerifyReply( const QTweetUser &user );
    void checkTimerFired();
    void connectTimerFired();
    void friendsTimelineStatuses( const QList< QTweetStatus > &statuses );
    void mentionsStatuses( const QList< QTweetStatus > &statuses );
    void pollDirectMessages();
    void directMessages( const QList< QTweetDMStatus > &messages );
    void directMessagePosted( const QTweetDMStatus &message );
    void directMessagePostError( QTweetNetBase::ErrorCode errorCode, const QString &message );
    void directMessageDestroyed( const QTweetDMStatus &message );
    void registerOffer( const QString &screenName, const QHash< QString, QVariant > &peerdata );
    void sendOffer( const QString &screenName, const QHash< QString, QVariant > &peerdata );
    void makeConnection( const QString &screenName, const QHash< QString, QVariant > &peerdata );
    void fetchAvatar( const QString &screenName );
    void avatarUserDataSlot( const QTweetUser &user );
    void profilePicReply();

private:
    bool refreshTwitterAuth();
    void parseGotTomahawk( const QRegExp &regex, const QString &screenName, const QString &text );
    // handle per-plugin config
    QString twitterScreenName() const;
    void setTwitterScreenName( const QString& screenName );
    QString twitterOAuthToken() const;
    void setTwitterOAuthToken( const QString& oauthtoken );
    QString twitterOAuthTokenSecret() const;
    void setTwitterOAuthTokenSecret( const QString& oauthtokensecret );
    qint64 twitterCachedFriendsSinceId() const;
    void setTwitterCachedFriendsSinceId( qint64 sinceid );
    qint64 twitterCachedMentionsSinceId() const;
    void setTwitterCachedMentionsSinceId( qint64 sinceid );
    qint64 twitterCachedDirectMessagesSinceId() const;
    void setTwitterCachedDirectMessagesSinceId( qint64 sinceid );
    QHash<QString, QVariant> twitterCachedPeers() const;
    void setTwitterCachedPeers( const QHash<QString, QVariant> &cachedPeers );
    bool twitterAutoConnect() const;
    void setTwitterAutoConnect( bool autoConnect );

    QWeakPointer< TomahawkOAuthTwitter > m_twitterAuth;
    QWeakPointer< QTweetFriendsTimeline > m_friendsTimeline;
    QWeakPointer< QTweetMentions > m_mentions;
    QWeakPointer< QTweetDirectMessages > m_directMessages;
    QWeakPointer< QTweetDirectMessageNew > m_directMessageNew;
    QWeakPointer< QTweetDirectMessageDestroy > m_directMessageDestroy;

    bool m_isAuthed;
    QTimer m_checkTimer;
    QTimer m_connectTimer;
    qint64 m_cachedFriendsSinceId;
    qint64 m_cachedMentionsSinceId;
    qint64 m_cachedDirectMessagesSinceId;
    QHash< QString, QVariant > m_cachedPeers;
    QHash< QString, QPixmap > m_cachedAvatars;
    QSet<QString> m_keyCache;
    bool m_finishedFriends;
    bool m_finishedMentions;
    ConnectionState m_state;

    QWeakPointer<TwitterConfigWidget > m_configWidget;

    // for settings access
    friend class TwitterConfigWidget;
};

#endif
