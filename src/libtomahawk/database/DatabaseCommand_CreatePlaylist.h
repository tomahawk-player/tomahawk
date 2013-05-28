/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef DATABASECOMMAND_CREATEPLAYLIST_H
#define DATABASECOMMAND_CREATEPLAYLIST_H

// #include "Typedefs.h"
#include "DatabaseCommandLoggable.h"
// #include "qjson/qobjecthelper.h"

#include "playlist_ptr.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_CreatePlaylist : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariant playlist READ playlistV WRITE setPlaylistV )

public:
    explicit DatabaseCommand_CreatePlaylist( QObject* parent = 0 );
    explicit DatabaseCommand_CreatePlaylist( const Tomahawk::source_ptr& author, const Tomahawk::playlist_ptr& playlist );
    virtual ~DatabaseCommand_CreatePlaylist();

    QString commandname() const { return "createplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    QVariant playlistV() const;

    void setPlaylistV( const QVariant& v )
    {
        m_v = v;
    }

protected:
    void createPlaylist( DatabaseImpl* lib, bool dynamic = false );

    virtual bool report() { return m_report; }

    void setPlaylist( const Tomahawk::playlist_ptr& playlist );

    QVariant m_v;

private:
    Tomahawk::playlist_ptr m_playlist;
    bool m_report; // call Playlist::reportCreated?
};

#endif // DATABASECOMMAND_CREATEPLAYLIST_H
