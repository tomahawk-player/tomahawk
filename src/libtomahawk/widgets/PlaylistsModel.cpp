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

#include "PlaylistsModel_p.h"

#include "widgets/RecentlyPlayedPlaylistsModel.h"
#include "Track.h"

namespace Tomahawk {


PlaylistsModel::PlaylistsModel( const QList<playlist_ptr>& playlists, QObject* parent )
    : QAbstractListModel(parent)
    , d_ptr( new PlaylistsModelPrivate( this, playlists ) )
{
    updateArtists();
}


PlaylistsModel::~PlaylistsModel()
{
}


QVariant
PlaylistsModel::data( const QModelIndex& index, int role ) const
{
    Q_D( const PlaylistsModel );

    if ( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    playlist_ptr pl = d->playlists.value( index.row() );
    switch( role )
    {
    case Qt::DisplayRole:
        return pl->title();
    case RecentlyPlayedPlaylistsModel::PlaylistRole:
        return QVariant::fromValue< Tomahawk::playlist_ptr >( pl );
    case RecentlyPlayedPlaylistsModel::ArtistRole:
    {
        return d->artists[pl];
    }
    case RecentlyPlayedPlaylistsModel::PlaylistTypeRole:
    {
        if ( !pl.dynamicCast< Tomahawk::DynamicPlaylist >().isNull() )
        {
            dynplaylist_ptr dynp = pl.dynamicCast< Tomahawk::DynamicPlaylist >();
            if ( dynp->mode() == Static )
                return RecentlyPlayedPlaylistsModel::AutoPlaylist;
            else if ( dynp->mode() == OnDemand )
                return RecentlyPlayedPlaylistsModel::Station;
        }
        else
        {
            return RecentlyPlayedPlaylistsModel::StaticPlaylist;
        }
    }
    case RecentlyPlayedPlaylistsModel::DynamicPlaylistRole:
    {
        dynplaylist_ptr dynp = pl.dynamicCast< Tomahawk::DynamicPlaylist >();
        return QVariant::fromValue< Tomahawk::dynplaylist_ptr >( dynp );
    }
    case RecentlyPlayedPlaylistsModel::TrackCountRole:
    {
        if ( !pl.dynamicCast< Tomahawk::DynamicPlaylist >().isNull() && pl.dynamicCast< Tomahawk::DynamicPlaylist >()->mode() == OnDemand )
            return QString( QChar( 0x221E ) );
        else
            return pl->entries().count();
    }
    default:
        return QVariant();
    }
}


int
PlaylistsModel::rowCount( const QModelIndex& ) const
{
    Q_D( const PlaylistsModel );
    return d->playlists.count();
}


void
PlaylistsModel::updateArtists()
{
    Q_D( PlaylistsModel );
    d->artists.clear();

    foreach ( playlist_ptr playlist, d->playlists )
    {
        QStringList artists;

        foreach ( const Tomahawk::plentry_ptr& entry, playlist->entries() )
        {
            if ( !artists.contains( entry->query()->track()->artist() ) )
                artists << entry->query()->track()->artist();
        }

        d->artists[ playlist ] = artists.join( ", " );
    }
}


} // namespace Tomahawk
