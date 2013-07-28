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

#include "PlaylistTemplate_p.h"

#include "Source.h"

Tomahawk::PlaylistTemplate::PlaylistTemplate(const Tomahawk::source_ptr& author, const QString &guid, const QString &title, const QString &info, const QString &creator, bool shared, const QList<Tomahawk::query_ptr> &queries)
    : QObject( 0 )
    , d_ptr( new PlaylistTemplatePrivate( this, author, guid, title, info, creator, shared, queries ) )
{
}


Tomahawk::PlaylistTemplate::~PlaylistTemplate()
{
    tLog() << Q_FUNC_INFO;
    delete d_ptr;
}


Tomahawk::playlist_ptr
Tomahawk::PlaylistTemplate::get()
{
    Q_D( PlaylistTemplate );

    if ( d->playlist.isNull() )
    {
        // First try to load the playlist if already exists
        d->playlist = Playlist::get( d->guid );
    }

    if ( d->playlist.isNull() )
    {
        // This playlist does not exist yet, so create it.
        d->playlist = Playlist::create( d->author, d->guid, d->title, d->info, d->creator, d->shared, d->queries );
    }

    return d->playlist;
}


QList<Tomahawk::query_ptr>
Tomahawk::PlaylistTemplate::tracks() const
{
    Q_D( const PlaylistTemplate );

    return d->queries;
}

Tomahawk::PlaylistTemplate::PlaylistTemplate( Tomahawk::PlaylistTemplatePrivate* d )
    : QObject( 0 )
    , d_ptr( d )
{
}
