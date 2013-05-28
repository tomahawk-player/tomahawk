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

#ifndef DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
#define DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H

// #include "Typedefs.h"
#include "dynplaylist_ptr.h"
#include "DatabaseCommand_CreatePlaylist.h"

/**
 * Create a new dynamic playlist in the database, based on an existing playlist.
 *
 * If autoLoad is true, this playlist will *not* show up in the sidebar under the playlist tree, and
 *  it will *not* be replicated to peers. It is useful to show a "specially crafted" playlist in other places
 */

class DatabaseCommand_CreateDynamicPlaylist : public DatabaseCommand_CreatePlaylist
{
    Q_OBJECT
    Q_PROPERTY( QVariant playlist READ playlistV WRITE setPlaylistV )

public:
    explicit DatabaseCommand_CreateDynamicPlaylist( QObject* parent = 0 );
    explicit DatabaseCommand_CreateDynamicPlaylist( const Tomahawk::source_ptr& author, const Tomahawk::dynplaylist_ptr& playlist, bool autoLoad = true );
    virtual ~DatabaseCommand_CreateDynamicPlaylist();

    QString commandname() const { return "createdynamicplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    virtual bool loggable() const { return m_autoLoad; }

    QVariant playlistV() const;

    void setPlaylistV( const QVariant& v )
    {
        m_v = v;
    }

protected:
    virtual bool report() { return m_autoLoad; }

private:
    Tomahawk::dynplaylist_ptr m_playlist;
    bool m_autoLoad;
};

#endif // DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
