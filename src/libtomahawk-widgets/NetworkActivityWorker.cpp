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
#include "database/DatabaseCommand_TrendingTracks.h"
#include "database/DatabaseImpl.h"
#include "NetworkActivityWidget.h"

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
        DatabaseCommand_LoadAllSources* dbcmd = new DatabaseCommand_LoadAllSources();
        connect( dbcmd, SIGNAL( done( QList<Tomahawk::source_ptr> ) ),
                 SLOT( allSourcesReceived( QList<Tomahawk::source_ptr> ) ),
                 Qt::QueuedConnection);
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
}


void
NetworkActivityWorker::allPlaylistsReceived( const QList<playlist_ptr>& playlists )
{
    Q_D( NetworkActivityWorker );
    d->sourcesToLoad--;
    d->playlists.append( playlists );

    if ( d->sourcesToLoad == 0 )
    {
        // Load all playlist entries
        foreach( playlist_ptr playlist, d->playlists )
        {
            if ( !playlist->loaded() )
            {
                d->playlistsRevisionToLoad++;
                connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
                         SLOT( revisionLoaded( Tomahawk::PlaylistRevision ) ),
                         Qt::QueuedConnection );
                playlist->loadRevision();
            }
        }
        checkRevisionLoadedDone();
    }
}


void
NetworkActivityWorker::allSourcesReceived( const QList<source_ptr>& sources )
{
    Q_D( NetworkActivityWorker );
    d->sourcesToLoad = sources.count();

    foreach ( const source_ptr& source, sources)
    {
        DatabaseCommand_LoadAllPlaylists* dbcmd = new DatabaseCommand_LoadAllPlaylists( source );
        connect( dbcmd, SIGNAL( done( QList<Tomahawk::playlist_ptr> ) ),
                 SLOT( allPlaylistsReceived ( QList<Tomahawk::playlist_ptr> ) ),
                 Qt::QueuedConnection );
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
}


void
NetworkActivityWorker::playtime( uint playtime )
{
    Q_D( NetworkActivityWorker );
    d->playlistCount.insert( playtime, d->playlistStack.pop() );
    calculateNextPlaylist();
}


void
NetworkActivityWorker::revisionLoaded(PlaylistRevision revision)
{
    Q_UNUSED( revision );
    Q_D( NetworkActivityWorker );
    d->playlistsRevisionToLoad--;
    checkRevisionLoadedDone();
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
NetworkActivityWorker::calculateNextPlaylist()
{
    Q_D( NetworkActivityWorker );
    if ( !d->playlistStack.isEmpty() )
    {
        DatabaseCommand_CalculatePlaytime* dbcmd = new DatabaseCommand_CalculatePlaytime( d->playlistStack.top(), QDateTime::currentDateTime().addDays( -7 ), QDateTime::currentDateTime() );
        connect( dbcmd, SIGNAL( done( uint ) ), SLOT( playtime( uint ) ), Qt::QueuedConnection );
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }
    else
    {
        d->playlistCount.remove( 0 );
        QList<playlist_ptr> playlists;
        QMapIterator<uint, playlist_ptr> iter (d->playlistCount);
        iter.toBack();
        while (iter.hasPrevious() && (uint)playlists.size() < Widgets::NetworkActivityWidget::numberOfHotPlaylists )
        {
            iter.previous();
            playlists << iter.value();
        }
        emit hotPlaylists( playlists );
        d->hotPlaylistsDone = true;
        checkDone();
    }
}

void
NetworkActivityWorker::checkRevisionLoadedDone()
{
    Q_D( NetworkActivityWorker );
    if ( d->playlistsRevisionToLoad == 0 )
    {
        foreach (playlist_ptr playlist, d->playlists) {
            d->playlistStack.push( playlist );
        }
        calculateNextPlaylist();
    }
}

void
NetworkActivityWorker::checkDone()
{
    Q_D( NetworkActivityWorker );
    if ( d->trendingTracksDone && d->hotPlaylistsDone )
    {
        emit finished();
    }
}

} // namespace Widgets

} // namespace Tomahawk
