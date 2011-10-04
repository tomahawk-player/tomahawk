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
#include "accounts/account.h"
#include "tomahawkoauthtwitter.h"

#define MYNAME "SIPTWITTER"

class SIPDLLEXPORT TwitterSipPlugin : public SipPlugin
{
    Q_OBJECT

public:
    TwitterSipPlugin( Tomahawk::Accounts::Account *account );

    virtual ~TwitterSipPlugin() {}

    virtual bool isValid() const;
    virtual ConnectionState connectionState() const;

public slots:
    virtual bool connectPlugin();
    void disconnectPlugin();
    void refreshProxy();
    void configurationChanged();

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
    void registerOffers( const QStringList &peerList );
    void registerOffer( const QString &screenName, const QVariantHash &peerdata );
    void sendOffer( const QString &screenName, const QVariantHash &peerdata );
    void makeConnection( const QString &screenName, const QVariantHash &peerdata );
    void fetchAvatar( const QString &screenName );
    void avatarUserDataSlot( const QTweetUser &user );
    void profilePicReply();

private:
    inline void syncConfig() { m_account->setCredentials( m_credentials ); m_account->setConfiguration( m_configuration ); m_account->syncConfig(); }
    bool refreshTwitterAuth();
    void parseGotTomahawk( const QRegExp &regex, const QString &screenName, const QString &text );
    // handle per-plugin config
    QString twitterOAuthToken() const;
    void setTwitterOAuthToken( const QString& oauthtoken );
    QString twitterOAuthTokenSecret() const;
    void setTwitterOAuthTokenSecret( const QString& oauthtokensecret );

    QWeakPointer< TomahawkOAuthTwitter > m_twitterAuth;
    QWeakPointer< QTweetFriendsTimeline > m_friendsTimeline;
    QWeakPointer< QTweetMentions > m_mentions;
    QWeakPointer< QTweetDirectMessages > m_directMessages;
    QWeakPointer< QTweetDirectMessageNew > m_directMessageNew;
    QWeakPointer< QTweetDirectMessageDestroy > m_directMessageDestroy;

    QVariantHash m_configuration;
    QVariantHash m_credentials;
    
    bool m_isAuthed;
    QTimer m_checkTimer;
    QTimer m_connectTimer;
    QTimer m_dmPollTimer;
    qint64 m_cachedFriendsSinceId;
    qint64 m_cachedMentionsSinceId;
    qint64 m_cachedDirectMessagesSinceId;
    QVariantHash m_cachedPeers;
    QHash< QString, QPixmap > m_cachedAvatars;
    QSet<QString> m_keyCache;
    ConnectionState m_state;

    QWeakPointer<TwitterConfigWidget > m_configWidget;

    // for settings access
    friend class TwitterConfigWidget;
};

#endif
