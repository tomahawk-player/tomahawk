/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef SPOTIFYPLAYLISTUPDATER_H
#define SPOTIFYPLAYLISTUPDATER_H

#include "playlist/PlaylistUpdaterInterface.h"
#include "utils/Closure.h"
#include "DllMacro.h"
#include "Typedefs.h"

#include <QQueue>
#include <QVariant>

namespace Tomahawk {
namespace Accounts {
    class SpotifyAccount;
}
}

class DLLEXPORT SpotifyPlaylistUpdater : public Tomahawk::PlaylistUpdaterInterface
{
    Q_OBJECT

    friend class Tomahawk::Accounts::SpotifyAccount;
public:
    SpotifyPlaylistUpdater( Tomahawk::Accounts::SpotifyAccount* acct, const QString& revid, const QString& spotifyId, const Tomahawk::playlist_ptr& pl );

    virtual ~SpotifyPlaylistUpdater();

    virtual QString type() const;
    virtual void updateNow() {}

#ifndef ENABLE_HEADLESS
    virtual QWidget* configurationWidget() const { return 0; }
    virtual QPixmap typeIcon() const;
#endif

    bool sync() const;
    void setSync( bool sync );
    bool subscribed() const;
    // actually change the subscribed value in spotify
    void setSubscribed( bool subscribed );
    // Just set the subscribed flag
    void setSubscribedStatus( bool subscribed );
    bool canSubscribe() const;
    void setCanSubscribe( bool canSub );
    void setSubscribers( int numSubscribers );
    int subscribers() const { return m_subscribers; }
    // Collaborative actions
    void setOwner( bool owner );
    bool owner() const;
    bool collaborative() const;
    void setCollaborative( bool collaborative );

    QString spotifyId() const { return m_spotifyId; }

    virtual bool hasCustomDeleter() const { return true; }
    virtual Tomahawk::PlaylistDeleteQuestions deleteQuestions() const;
    virtual void setQuestionResults( const QMap< int, bool > results );

    void remove( bool askToDeletePlaylist = true );
public slots:
    /// Spotify callbacks when we are directly instructed from the resolver
    void spotifyTracksAdded( const QVariantList& tracks, const QString& startPosId, const QString& newRev, const QString& oldRev );
    void spotifyTracksRemoved( const QVariantList& tracks, const QString& newRev, const QString& oldRev );
    void spotifyTracksMoved( const QVariantList& tracks, const QString& newStartPos, const QString& newRev, const QString& oldRev );
    void spotifyPlaylistRenamed( const QString& title, const QString& newRev, const QString& oldRev  );

    void tomahawkTracksInserted( const QList<Tomahawk::plentry_ptr>& ,int );
    void tomahawkTracksRemoved( const QList<Tomahawk::query_ptr>& );
    void tomahawkTracksMoved( const QList<Tomahawk::plentry_ptr>& ,int );
    void tomahawkPlaylistRenamed( const QString&, const QString& );

private slots:
    // SpotifyResolver message handlers, all take msgtype, msg as argument
    void onTracksInsertedReturn( const QString& msgType, const QVariantMap& msg, const QVariant& extraData );
    void onTracksRemovedReturn( const QString& msgType, const QVariantMap& msg, const QVariant& extraData );
    void onTracksMovedReturn( const QString& msgType, const QVariantMap& msg, const QVariant& extraData );

    void unsyncOrDelete( bool toDelete );

    void playlistRevisionLoaded();
private:
    void init();
    void saveToSettings();

    /// Finds the nearest spotify id from pos to the beginning of the playlist
    QString nearestSpotifyTrack( const QList< Tomahawk::plentry_ptr >& entries,  int pos );
    QVariantList plentryToVariant( const QList< Tomahawk::plentry_ptr >& entries );

    static QVariantList queriesToVariant( const QList< Tomahawk::query_ptr >& queries );
    static QVariant queryToVariant( const Tomahawk::query_ptr& query );
    static QList< Tomahawk::query_ptr > variantToQueries( const QVariantList& list );

    QPointer<Tomahawk::Accounts::SpotifyAccount> m_spotify;
    QString m_latestRev, m_spotifyId;
    QList< Tomahawk::plentry_ptr > m_waitingForIds;

    bool m_blockUpdatesForNextRevision;
    bool m_sync;
    bool m_subscribed;
    bool m_canSubscribe;
    bool m_isOwner;
    bool m_collaborative;
    int m_subscribers;

    QQueue<_detail::Closure*> m_queuedOps;
#ifndef ENABLE_HEADLESS
    static QPixmap* s_typePixmap;
#endif
};


class DLLEXPORT SpotifyUpdaterFactory : public Tomahawk::PlaylistUpdaterFactory
{
public:
    SpotifyUpdaterFactory()  {}

    virtual Tomahawk::PlaylistUpdaterInterface* create( const Tomahawk::playlist_ptr& pl, const QVariantHash& settings );
    virtual QString type() const { return "spotify"; }

private:
    QPointer<Tomahawk::Accounts::SpotifyAccount> m_account;
};

#endif // SPOTIFYPLAYLISTUPDATER_H
