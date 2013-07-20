/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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
#ifndef DATABASECOMMAND_LOADALLPLAYLIST_H
#define DATABASECOMMAND_LOADALLPLAYLIST_H

#include "DatabaseCommand.h"

namespace Tomahawk
{

class DatabaseCommand_LoadAllPlaylistsPrivate;

class DLLEXPORT DatabaseCommand_LoadAllPlaylists : public DatabaseCommand
{
    Q_OBJECT

public:
    enum SortOrder {
        None = 0,
        ModificationTime = 1
    };
    enum SortAscDesc {
        NoOrder = 0,
        Ascending = 1,
        Descending = 2
    };

    explicit DatabaseCommand_LoadAllPlaylists( const Tomahawk::source_ptr& s, QObject* parent = 0 );

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadallplaylists"; }

    void setLimit( unsigned int limit );
    void setSortOrder( SortOrder order );
    void setSortDescending( bool descending );

    /**
     * By default, only playlists with no revision will be loaded as fully
     * loading a revision is a lot of work which cannot be integrated into
     * one query.
     *
     * Often one only needs to know the trackIds of a playlist, not the
     * whole revision information.
     */
    void setReturnPlEntryIds( bool returnPlEntryIds );

signals:
    void done( const QList<Tomahawk::playlist_ptr>& playlists );

    /**
     * This signal is only emitted if returnTrackIds == true.
     */
    void done( const QHash< Tomahawk::playlist_ptr, QStringList >& playlists );

private:
    Q_DECLARE_PRIVATE( DatabaseCommand_LoadAllPlaylists )
};

} // namespace Tomahawk

#endif // DATABASECOMMAND_LOADALLPLAYLIST_H
