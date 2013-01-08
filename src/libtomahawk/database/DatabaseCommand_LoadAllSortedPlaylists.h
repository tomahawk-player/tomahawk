/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DATABASECOMMAND_LOADALLSORTEDPLAYLISTS_H
#define DATABASECOMMAND_LOADALLSORTEDPLAYLISTS_H

#include "DatabaseCommand.h"
#include "DatabaseCommand_LoadAllPlaylists.h"

/**
 * Loads *all* playlists, automatic playlists, and stations. Another dbcmd because otherwise loading them all
 * is fragmented across 3 dbcmds with a different interface.
 *
 * You probably want to limit / sort the output.
 */
class DatabaseCommand_LoadAllSortedPlaylists : public DatabaseCommand
{
    Q_OBJECT

public:
    // don't macros rock... not
    typedef QPair<int,QString> SourcePlaylistPair;
    explicit DatabaseCommand_LoadAllSortedPlaylists( const Tomahawk::source_ptr& s, QObject* parent = 0 )
        : DatabaseCommand( s, parent )
        , m_limitAmount( 0 )
        , m_sortOrder( DatabaseCommand_LoadAllPlaylists::None )
        , m_sortAscDesc( DatabaseCommand_LoadAllPlaylists::NoOrder )
    {
        qRegisterMetaType<QList<DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair> >("QList<DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair>");
    }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadallsortedplaylists"; }

    void setLimit( unsigned int limit ) { m_limitAmount = limit; }
    void setSortOrder( DatabaseCommand_LoadAllPlaylists::SortOrder order ) { m_sortOrder = order; }
    void setSortAscDesc( DatabaseCommand_LoadAllPlaylists::SortAscDesc asc  ) { m_sortAscDesc = asc; }

signals:
    void done( const QList<DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair>& playlistGuids ); // QPair< sourceid, playlistguid>

private:
    unsigned int m_limitAmount;
    DatabaseCommand_LoadAllPlaylists::SortOrder m_sortOrder;
    DatabaseCommand_LoadAllPlaylists::SortAscDesc m_sortAscDesc;
};

//FIXME: Qt5: this fails with Qt5, is it needed at all? It compiles fine without in Qt4 as well
// Q_DECLARE_METATYPE(QList<DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair>)

#endif // DATABASECOMMAND_LOADALLSORTEDPLAYLISTS_H
