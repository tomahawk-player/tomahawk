/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "SpotifyPlaylistUpdater.h"

#include "accounts/AccountManager.h"
#include "SpotifyAccount.h"

using namespace Tomahawk;
using namespace Accounts;

Tomahawk::PlaylistUpdaterInterface*
SpotifyUpdaterFactory::create( const Tomahawk::playlist_ptr& pl )
{
    if ( !m_account )
    {
        // Find the spotify account
        foreach ( Account* account, AccountManager::instance()->accounts() )
        {
            if ( SpotifyAccount* spotify = qobject_cast< SpotifyAccount* >( account ) )
            {
                m_account = spotify;
                break;
            }
        }
    }

    if ( !m_account )
    {
        qWarning() << "Found a spotify updater with no spotify account... ignoreing for now!!";
        return 0;
    }

    // Register the updater with the account
    const QString spotifyId = TomahawkSettings::instance()->value( QString( "playlistupdaters/%1/spotifyId" ).arg( pl->guid() ) ).toString();
    Q_ASSERT( !spotifyId.isEmpty() );
    SpotifyPlaylistUpdater* updater = new SpotifyPlaylistUpdater( m_account, pl );
    m_account->registerUpdaterForPlaylist( spotifyId, updater );

    return updater;
}


SpotifyPlaylistUpdater::SpotifyPlaylistUpdater( SpotifyAccount* acct, const playlist_ptr& pl )
    : PlaylistUpdaterInterface( pl )
    , m_spotify( acct )
    , m_sync( false )
{
    // values will be loaded from settings
    init();
}


SpotifyPlaylistUpdater::SpotifyPlaylistUpdater( SpotifyAccount* acct, const QString& revid, const QString& spotifyId, const playlist_ptr& pl )
    : PlaylistUpdaterInterface( pl )
    , m_spotify( acct )
    , m_latestRev( revid )
    , m_spotifyId( spotifyId )
    , m_blockUpdatesForNextRevision( false )
    , m_sync( false )
{
    init();
}


void
SpotifyPlaylistUpdater::init()
{

    connect( playlist().data(), SIGNAL( tracksInserted( QList<Tomahawk::plentry_ptr>, int ) ), this, SLOT( tomahawkTracksInserted( QList<Tomahawk::plentry_ptr>, int ) ) );
    connect( playlist().data(), SIGNAL( tracksRemoved( QList<Tomahawk::query_ptr> ) ), this, SLOT( tomahawkTracksRemoved( QList<Tomahawk::query_ptr> ) ) );
    // TODO reorders in a playlist
}


SpotifyPlaylistUpdater::~SpotifyPlaylistUpdater()
{

}

void
SpotifyPlaylistUpdater::loadFromSettings( const QString& group )
{
    m_latestRev = TomahawkSettings::instance()->value( QString( "%1/latestrev" ).arg( group ) ).toString();
    m_sync = TomahawkSettings::instance()->value( QString( "%1/sync" ).arg( group ) ).toBool();
    m_spotifyId = TomahawkSettings::instance()->value( QString( "%1/spotifyId" ).arg( group ) ).toString();
}

void
SpotifyPlaylistUpdater::removeFromSettings( const QString& group ) const
{
    TomahawkSettings::instance()->remove( QString( "%1/latestrev" ).arg( group ) );
    TomahawkSettings::instance()->remove( QString( "%1/sync" ).arg( group ) );
    TomahawkSettings::instance()->remove( QString( "%1/spotifyId" ).arg( group ) );
}


void
SpotifyPlaylistUpdater::saveToSettings( const QString& group ) const
{
    TomahawkSettings::instance()->setValue( QString( "%1/latestrev" ).arg( group ), m_latestRev );
    TomahawkSettings::instance()->setValue( QString( "%1/sync" ).arg( group ), m_sync );
    TomahawkSettings::instance()->setValue( QString( "%1/spotifyId" ).arg( group ), m_spotifyId );
}


QString
SpotifyPlaylistUpdater::type() const
{
    return "spotify";
}


void
SpotifyPlaylistUpdater::setSync( bool sync )
{
    m_sync = sync;
}


bool
SpotifyPlaylistUpdater::sync() const
{
    return m_sync;
}


void
SpotifyPlaylistUpdater::spotifyTracksAdded( const QVariantList& tracks, const QString& startPosId, const QString& newRev, const QString& oldRev )
{
    const QList< query_ptr > queries = variantToQueries( tracks );

    qDebug() << Q_FUNC_INFO << "inserting tracks in middle of tomahawk playlist, from spotify command!" << tracks << startPosId << newRev << oldRev;
    // Uh oh, dont' want to get out of sync!!
//     Q_ASSERT( m_latestRev == oldRev );
//     m_latestRev = newRev;

    // Find the position of the track to insert from
    int pos = -1;
    QList< plentry_ptr > entries = playlist()->entries();
    for ( int i = 0; i < entries.size(); i++ )
    {
        if ( entries[ i ]->annotation() == startPosId )
        {
            pos = i;
            break;
        }
    }
    pos++; // We found index of item before, so get index of new item.

    if ( pos == -1 || pos > entries.size() )
        pos = entries.size();

    qDebug() << Q_FUNC_INFO << "inserting tracks at position:" << pos;

    m_blockUpdatesForNextRevision = true;
    playlist()->insertEntries( queries, pos, playlist()->currentrevision() );
}


void
SpotifyPlaylistUpdater::spotifyTracksRemoved( const QVariantList& trackIds, const QString& newRev, const QString& oldRev )
{
    qDebug() << Q_FUNC_INFO << "remove tracks in middle of tomahawk playlist, from spotify command!" << trackIds << newRev << oldRev;
    // Uh oh, dont' want to get out of sync!!
//     Q_ASSERT( m_latestRev == oldRev );
//     m_latestRev = newRev;

    QList< plentry_ptr > entries = playlist()->entries();

    // Collect list of tracks to remove (can't remove in-place as that might modify the indices)
    QList<plentry_ptr> toRemove;
    foreach( const QVariant trackIdV, trackIds )
    {
        const QString id = trackIdV.toString();
        if ( id.isEmpty() )
        {
            qWarning() << Q_FUNC_INFO << "Tried to get track id to remove, but either couldn't convert to qstring:" << trackIdV;
            continue;
        }

        foreach ( const plentry_ptr& entry, entries )
        {
            if ( entry->annotation() == id )
            {
                toRemove << entry;
                break;
            }
            // TODO
            // We have to try a fuzzy comparison as well, because if we added a track from tomahawk in this session to this spotify playlist,
            // we don't have a spotify id for it (the track came from here, not from spotify).
            // OR we have to get spotify ids for each track when it's added to a spotify-synced playlist.
            // TODO
        }
    }


    // Now remove them all
    foreach( const plentry_ptr& torm, toRemove )
        entries.removeAll( torm );

    const int sizeDiff = playlist()->entries().size() - entries.size();
    qDebug() << "We were asked to delete:" << trackIds.size() << "tracks from the playlist, and we deleted:" << sizeDiff;
    if ( trackIds.size() != ( playlist()->entries().size() - entries.size() ) )
        qWarning() << "========================= Failed to delete all the tracks we were asked for!! Didn't find some indicesss... ===================";

    if ( sizeDiff > 0 )
    {
        m_blockUpdatesForNextRevision = true;
        playlist()->createNewRevision( uuid(), playlist()->currentrevision(), entries );
    }
}


void
SpotifyPlaylistUpdater::spotifyTracksMoved( const QVariantList& tracks, const QString& newRev, const QString& oldRev )
{
    // TODO
}


void
SpotifyPlaylistUpdater::tomahawkTracksInserted( const QList< plentry_ptr >& tracks, int pos )
{
    if ( m_blockUpdatesForNextRevision )
    {
        qDebug() << "Ignoring tracks inserted message since we just did an insert ourselves!";
        m_blockUpdatesForNextRevision = false;
        return;
    }

    // Notify the resolver that we've updated
    qDebug() << Q_FUNC_INFO  << "updating spotify resolver with inserted tracks at:" << pos << tracks;
    QVariantMap msg;
    msg[ "_msgtype" ] = "addTracksToPlaylist";
    msg[ "oldrev" ] = m_latestRev;

    // Find the trackid of the nearest spotify track
    QList< plentry_ptr > plTracks = playlist()->entries();
    Q_ASSERT( pos-1 < plTracks.size() );

    for ( int i = pos-1; i >= 0; i-- )
    {
        if ( !plTracks[ i ]->annotation().isEmpty() && plTracks[ i ]->annotation().contains( "spotify:track") )
        {
            msg[ "startPosition" ] = plTracks[ i ]->annotation();
            break;
        }
    }

    msg[ "playlistid" ] = m_spotifyId;

    QVariantList tracksJson;
    foreach ( const plentry_ptr& ple, tracks )
    {
        const query_ptr q = ple->query();
        if ( q.isNull() )
        {
            qDebug() << "Got null query_ptr in plentry_ptr!!!" << ple;
            continue;
        }

        tracksJson << queryToVariant( q );
    }
    msg[ "tracks" ] = tracksJson;

    m_spotify->sendMessage( msg, this, "onTracksInsertedReturn" );
}


void
SpotifyPlaylistUpdater::onTracksInsertedReturn( const QString& msgType, const QVariantMap& msg )
{
    const bool success = msg.value( "success" ).toBool();

    qDebug() << Q_FUNC_INFO << "GOT RETURN FOR tracksInserted call from spotify!" << msgType << msg << "Succeeded?" << success;
    m_latestRev = msg.value( "revid" ).toString();
}


void
SpotifyPlaylistUpdater::tomahawkTracksRemoved( const QList< query_ptr >& tracks )
{
    if ( m_blockUpdatesForNextRevision )
    {
        qDebug() << "Ignoring tracks removed message since we just did a remove ourselves!";
        m_blockUpdatesForNextRevision = false;
        return;
    }

    qDebug() << Q_FUNC_INFO  << "updating spotify resolver with removed tracks:" << tracks;
    QVariantMap msg;
    msg[ "_msgtype" ] = "removeTracksFromPlaylist";
    msg[ "playlistid" ] = m_spotifyId;
    msg[ "oldrev" ] = m_latestRev;
    msg[ "tracks" ] = queriesToVariant( tracks );

    m_spotify->sendMessage( msg, this, "onTracksRemovedReturn" );
}


void
SpotifyPlaylistUpdater::onTracksRemovedReturn( const QString& msgType, const QVariantMap& msg )
{
    const bool success = msg.value( "success" ).toBool();

    qDebug() << Q_FUNC_INFO << "GOT RETURN FOR tracksRemoved call from spotify!" << msgType << msg << "Succeeded?" << success;
    m_latestRev = msg.value( "revid" ).toString();
}


QVariantList
SpotifyPlaylistUpdater::queriesToVariant( const QList< query_ptr >& queries )
{
    QVariantList tracksJson;
    foreach ( const query_ptr& q, queries )
    {
        QVariantMap m;
        if ( q.isNull() )
            continue;
        tracksJson << queryToVariant( q );
    }

    return tracksJson;
}


QVariant
SpotifyPlaylistUpdater::queryToVariant( const query_ptr& query )
{
    QVariantMap m;
    m[ "track" ] = query->track();
    m[ "artist" ] = query->artist();
    m[ "album" ] = query->album();

    if ( !query->property( "annotation" ).isNull() )
        m[ "id" ] = query->property( "annotation" );

    return m;
}


QList< query_ptr >
SpotifyPlaylistUpdater::variantToQueries( const QVariantList& list )
{
    QList< query_ptr > queries;
    foreach ( const QVariant& blob, list )
    {
        QVariantMap trackMap = blob.toMap();
        const query_ptr q = Query::get( trackMap.value( "artist" ).toString(), trackMap.value( "track" ).toString(), trackMap.value( "album" ).toString(), uuid(), false );
        if ( trackMap.contains( "id" ) )
            q->setProperty( "annotation", trackMap.value( "id" ) );

        queries << q;
    }

    return queries;
}

