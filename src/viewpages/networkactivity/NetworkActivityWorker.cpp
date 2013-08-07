/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "NetworkActivityWorker_p.h"

#include "database/Database.h"
#include "database/DatabaseCommand_CalculatePlaytime.h"
#include "database/DatabaseCommand_LoadAllPlaylists.h"
#include "database/DatabaseCommand_LoadAllSources.h"
#include "database/DatabaseCommand_TrendingArtists.h"
#include "database/DatabaseCommand_TrendingTracks.h"
#include "database/DatabaseImpl.h"
#include "NetworkActivityWidget.h"

#include "utils/Logger.h"

#include <QDateTime>

namespace Tomahawk
{

namespace Widgets
{

NetworkActivityWorker::NetworkActivityWorker( QObject* parent )
    : QObject( parent )
    , d_ptr( new NetworkActivityWorkerPrivate( this ) )
{
}


NetworkActivityWorker::~NetworkActivityWorker()
{
}


void
NetworkActivityWorker::run()
{
    tLog() << Q_FUNC_INFO << QDateTime::currentDateTime().toTime_t();
    {
        // Load trending tracks
        qRegisterMetaType< QList< QPair< double,Tomahawk::track_ptr > > >("QList< QPair< double,Tomahawk::track_ptr > >");
        DatabaseCommand_TrendingTracks* dbcmd = new DatabaseCommand_TrendingTracks();
        dbcmd->setLimit( Tomahawk::Widgets::NetworkActivityWidget::numberOfTrendingTracks );
        connect( dbcmd, SIGNAL( done( QList< QPair< double,Tomahawk::track_ptr > >) ),
                 SLOT( trendingTracksReceived( QList< QPair< double,Tomahawk::track_ptr > > ) ),
                 Qt::QueuedConnection );
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
    {
        qRegisterMetaType< QList< QPair< double, Tomahawk::artist_ptr > > >("QList< QPair< double, Tomahawk::artist_ptr > >");
        DatabaseCommand_TrendingArtists* dbcmd = new DatabaseCommand_TrendingArtists();
        dbcmd->setLimit( Tomahawk::Widgets::NetworkActivityWidget::numberOfTrendingArtists );
        connect( dbcmd, SIGNAL( done( QList< QPair< double, Tomahawk::artist_ptr > > ) ),
                 SLOT( trendingArtistsReceived( QList< QPair< double, Tomahawk::artist_ptr > >) ),
                 Qt::QueuedConnection );
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
    {
        DatabaseCommand_LoadAllSources* dbcmd = new DatabaseCommand_LoadAllSources();
        connect( dbcmd, SIGNAL( done( QList<Tomahawk::source_ptr> ) ),
                 SLOT( allSourcesReceived( QList<Tomahawk::source_ptr> ) ),
                 Qt::QueuedConnection);
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
    tLog() << Q_FUNC_INFO << QDateTime::currentDateTime().toTime_t();
}


void
NetworkActivityWorker::allPlaylistsReceived( const QHash< Tomahawk::playlist_ptr, QStringList >& playlists )
{
    Q_D( NetworkActivityWorker );
    d->sourcesToLoad--;
    d->playlists.unite( playlists );

    if ( d->sourcesToLoad == 0 )
    {
        tLog() << Q_FUNC_INFO << QDateTime::currentDateTime().toTime_t();
        // Load all playlist entries

        foreach( playlist_ptr playlist, d->playlists.keys() )
        {
            d->playtimesToLoad++;
            DatabaseCommand_CalculatePlaytime* dbcmd = new DatabaseCommand_CalculatePlaytime( playlist, d->playlists.value( playlist ), QDateTime::currentDateTime().addDays( -7 ), QDateTime::currentDateTime() );
            connect( dbcmd, SIGNAL( done( Tomahawk::playlist_ptr, uint ) ), SLOT( playtime( Tomahawk::playlist_ptr, uint ) ) );
            Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
        }
    }
}


void
NetworkActivityWorker::allSourcesReceived( const QList<source_ptr>& sources )
{
    tLog() << Q_FUNC_INFO << QDateTime::currentDateTime().toTime_t();
    Q_D( NetworkActivityWorker );
    d->sourcesToLoad = sources.count();

    foreach ( const source_ptr& source, sources)
    {
        DatabaseCommand_LoadAllPlaylists* dbcmd = new DatabaseCommand_LoadAllPlaylists( source );
        dbcmd->setReturnPlEntryIds( true );
        connect( dbcmd, SIGNAL( done( QHash< Tomahawk::playlist_ptr, QStringList > ) ),
                 SLOT( allPlaylistsReceived ( QHash< Tomahawk::playlist_ptr, QStringList > ) ),
                 Qt::QueuedConnection );
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
}


void
NetworkActivityWorker::playlistLoaded(PlaylistRevision)
{
    Q_D( NetworkActivityWorker );

    d->playlistsToLoad--;
    checkHotPlaylistsDone();
}


void
NetworkActivityWorker::playtime( const Tomahawk::playlist_ptr& playlist, uint playtime )
{
    Q_D( NetworkActivityWorker );
    d->playtimesToLoad--;
    d->playlistCount.insert( playtime, playlist );

    if ( d->playtimesToLoad == 0 )
    {
        tLog() << Q_FUNC_INFO << QDateTime::currentDateTime().toTime_t();
        d->playlistCount.remove( 0 );
        QList<playlist_ptr> playlists;
        QMapIterator<uint, playlist_ptr> iter( d->playlistCount );
        iter.toBack();
        while (iter.hasPrevious() && (uint)playlists.size() < Widgets::NetworkActivityWidget::numberOfHotPlaylists )
        {
            iter.previous();
            Tomahawk::playlist_ptr playlist = iter.value();
            playlists << playlist;
            if ( !playlist->loaded() )
            {
                d->playlistsToLoad++;
                connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
                         SLOT( playlistLoaded( Tomahawk::PlaylistRevision ) ),
                         Qt::QueuedConnection );
                playlist->loadRevision();
            }
        }
        d->hotPlaylists = playlists;

        checkHotPlaylistsDone();
    }
}


void
NetworkActivityWorker::trendingArtistsReceived( const QList<QPair<double, artist_ptr> >& _artists )
{
    Q_D( NetworkActivityWorker );
    d->trendingArtistsDone = true;

    QList< artist_ptr > artists;
    QList< QPair< double, artist_ptr > >::const_iterator iter = _artists.constBegin();
    const QList< QPair< double, artist_ptr > >::const_iterator end = _artists.constEnd();
    for(; iter != end; ++iter)
    {
        artists << iter->second;
    }
    emit trendingArtists( artists );

    checkDone();
}


void
NetworkActivityWorker::trendingTracksReceived( const QList<QPair<double, Tomahawk::track_ptr> >& _tracks)
{
    Q_D( NetworkActivityWorker );
    d->trendingTracksDone = true;

    QList<track_ptr> tracks;
    QList< QPair< double, track_ptr > >::const_iterator iter = _tracks.constBegin();
    const QList< QPair< double, track_ptr > >::const_iterator end = _tracks.constEnd();
    for(; iter != end; ++iter)
    {
        tracks << iter->second;
    }
    emit trendingTracks( tracks );

    checkDone();
}

void
NetworkActivityWorker::checkDone()
{
    Q_D( NetworkActivityWorker );
    if ( d->trendingTracksDone && d->trendingArtistsDone && d->hotPlaylistsDone )
    {
        emit finished();
    }
}


void
NetworkActivityWorker::checkHotPlaylistsDone()
{
    Q_D(NetworkActivityWorker);

    if ( d->playlistsToLoad == 0 )
    {
        emit hotPlaylists( d->hotPlaylists );
        d->hotPlaylistsDone = true;
        checkDone();
    }
}


} // namespace Widgets

} // namespace Tomahawk
