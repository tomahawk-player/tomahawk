/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "accounts/AccountDllMacro.h"
#include "sip/SipPlugin.h"
#include "accounts/Account.h"
#include "accounts/twitter/TomahawkOAuthTwitter.h"

#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetnetbase.h>
#include <QTweetLib/qtweetfriendstimeline.h>
#include <QTweetLib/qtweetdirectmessages.h>
#include <QTweetLib/qtweetdirectmessagenew.h>
#include <QTweetLib/qtweetdirectmessagedestroy.h>
#include <QTweetLib/qtweetmentions.h>
#include <QTweetLib/qtweetdmstatus.h>

#include <QTimer>
#include <QPointer>
#include <QSet>


class ACCOUNTDLLEXPORT TwitterSipPlugin : public SipPlugin
{
    Q_OBJECT

public:
    TwitterSipPlugin( Tomahawk::Accounts::Account *account );

    virtual ~TwitterSipPlugin() {}

    virtual bool isValid() const;
    virtual Tomahawk::Accounts::Account::ConnectionState connectionState() const;
    virtual QString inviteString() const;

signals:
    void stateChanged( Tomahawk::Accounts::Account::ConnectionState );

public slots:
    virtual void connectPlugin();
    void disconnectPlugin();
    void configurationChanged();

    void sendMsg( const QString& peerId, const SipInfo& info )
    {
        Q_UNUSED( peerId );
        Q_UNUSED( info );
    }

    void broadcastMsg( const QString &msg )
    {
        Q_UNUSED( msg );
    }

    void addContact( const QString &peerId, const QString& msg = QString() )
    {
        Q_UNUSED( peerId );
        Q_UNUSED( msg );
    }

    void checkSettings();

private slots:
    void accountAuthenticated( const QPointer< TomahawkOAuthTwitter > &twitterAuth, const QTweetUser &user );
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
    void syncConfig();
    bool refreshTwitterAuth();
    void parseGotTomahawk( const QRegExp &regex, const QString &screenName, const QString &text );

    QPointer< TomahawkOAuthTwitter > m_cachedTwitterAuth;

    QPointer< QTweetFriendsTimeline > m_friendsTimeline;
    QPointer< QTweetMentions > m_mentions;
    QPointer< QTweetDirectMessages > m_directMessages;
    QPointer< QTweetDirectMessageNew > m_directMessageNew;
    QPointer< QTweetDirectMessageDestroy > m_directMessageDestroy;

    QVariantHash m_configuration;

    QTimer m_checkTimer;
    QTimer m_connectTimer;
    QTimer m_dmPollTimer;
    qint64 m_cachedFriendsSinceId;
    qint64 m_cachedMentionsSinceId;
    qint64 m_cachedDirectMessagesSinceId;
    QVariantHash m_cachedPeers;
    QHash< QString, QPixmap > m_cachedAvatars;
    QSet<QString> m_keyCache;
    Tomahawk::Accounts::Account::ConnectionState m_state;
};

#endif
