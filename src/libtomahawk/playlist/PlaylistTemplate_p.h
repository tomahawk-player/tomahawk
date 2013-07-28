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

#ifndef PLAYLISTTEMPLATE_P_H
#define PLAYLISTTEMPLATE_P_H

#include "playlist/PlaylistTemplate.h"

#include "Playlist.h"
#include "utils/Logger.h"

namespace Tomahawk
{

class PlaylistTemplatePrivate
{
public:
    PlaylistTemplatePrivate( PlaylistTemplate* q,
                             const source_ptr& _author,
                             const QString& _guid,
                             const QString& _title,
                             const QString& _info,
                             const QString& _creator,
                             bool _shared,
                             const QList<Tomahawk::query_ptr>& _queries )
        : q_ptr( q )
        , author( _author )
        , guid( _guid )
        , title( _title )
        , info( _info )
        , creator( _creator )
        , shared( _shared )
        , queries( _queries )
    {
    }

    Q_DECLARE_PUBLIC( PlaylistTemplate )
    PlaylistTemplate* q_ptr;

protected:
    source_ptr author;
    QString guid;
    QString title;
    QString info;
    QString creator;
    bool shared;
    QList<Tomahawk::query_ptr> queries;
    playlist_ptr playlist;
};

}

#endif // PLAYLISTTEMPLATE_P_H
