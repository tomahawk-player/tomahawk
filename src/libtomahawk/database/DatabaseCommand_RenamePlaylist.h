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

#ifndef DATABASECOMMAND_RENAMEPLAYLIST_H
#define DATABASECOMMAND_RENAMEPLAYLIST_H

#include "Typedefs.h"
#include "DatabaseCommandLoggable.h"

#include "DllMacro.h"

class DatabaseImpl;

class DLLEXPORT DatabaseCommand_RenamePlaylist : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString playlistguid READ playlistguid WRITE setPlaylistguid )
Q_PROPERTY( QString playlistTitle READ playlistTitle WRITE setPlaylistTitle )

public:
    explicit DatabaseCommand_RenamePlaylist( QObject* parent = 0 )
            : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_RenamePlaylist( const Tomahawk::source_ptr& source, const QString& playlistguid, const QString& playlistTitle );

    QString commandname() const { return "renameplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    QString playlistguid() const { return m_playlistguid; }
    void setPlaylistguid( const QString& s ) { m_playlistguid = s; }

    QString playlistTitle() const { return m_playlistTitle; }
    void setPlaylistTitle( const QString& s ) { m_playlistTitle = s; }

private:
    QString m_playlistguid;
    QString m_playlistTitle;
};

#endif // DATABASECOMMAND_RENAMEPLAYLIST_H
