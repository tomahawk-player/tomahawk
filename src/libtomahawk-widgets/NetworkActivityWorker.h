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

#pragma once
#ifndef TOMAHAWK_NETWORKACTIVITYWORKER_H
#define TOMAHAWK_NETWORKACTIVITYWORKER_H

#include "Playlist.h"
#include "Typedefs.h"

#include <QObject>

namespace Tomahawk
{

namespace Widgets
{

class NetworkActivityWorkerPrivate;

class NetworkActivityWorker : public QObject
{
    Q_OBJECT
public:
    explicit NetworkActivityWorker( QObject *parent = 0 );
    virtual ~NetworkActivityWorker();

public slots:
    void run();

signals:
    void trendingArtists( const QList< Tomahawk::artist_ptr >& artists );
    void trendingTracks( const QList<Tomahawk::track_ptr>& tracks );
    void hotPlaylists( const QList<Tomahawk::playlist_ptr>& playlists );
    void finished();

protected:
    QScopedPointer<NetworkActivityWorkerPrivate> d_ptr;

private slots:
    void allPlaylistsReceived( const QHash< Tomahawk::playlist_ptr, QStringList >& playlists );
    void allSourcesReceived( const QList< Tomahawk::source_ptr >& sources );
    void playlistLoaded( Tomahawk::PlaylistRevision );
    void playtime( const Tomahawk::playlist_ptr& playlist , uint playtime );
    void trendingArtistsReceived( const QList< QPair< double,Tomahawk::artist_ptr > >& tracks );
    void trendingTracksReceived( const QList< QPair< double,Tomahawk::track_ptr > >& tracks );

private:
    Q_DECLARE_PRIVATE( NetworkActivityWorker )

    void checkDone();
    void checkHotPlaylistsDone();
};

} // namespace Widgets

} // namespace Tomahawk

#endif // TOMAHAWK_NETWORKACTIVITYWORKER_H
