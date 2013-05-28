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

#ifndef DATABASECOMMAND_DELETEPLAYLIST_H
#define DATABASECOMMAND_DELETEPLAYLIST_H

// #include "DatabaseImpl.h"
#include "DatabaseCommandLoggable.h"
// #include "Source.h"
// #include "Typedefs.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_DeletePlaylist : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString playlistguid READ playlistguid WRITE setPlaylistguid )

public:
    explicit DatabaseCommand_DeletePlaylist( QObject* parent = 0 )
            : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_DeletePlaylist( const Tomahawk::source_ptr& source, const QString& playlistguid );

    QString commandname() const { return "deleteplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    QString playlistguid() const { return m_playlistguid; }
    void setPlaylistguid( const QString& s ) { m_playlistguid = s; }

protected:
    QString m_playlistguid;
};

#endif // DATABASECOMMAND_DELETEPLAYLIST_H
