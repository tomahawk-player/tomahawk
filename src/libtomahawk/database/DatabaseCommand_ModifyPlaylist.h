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

#ifndef DATABASECOMMAND_MODIFYPLAYLIST_H
#define DATABASECOMMAND_MODIFYPLAYLIST_H

// #include <QObject>
// #include <QVariantMap>
//
// #include "Typedefs.h"
#include "plentry_ptr.h"

#include "DatabaseCommand.h"

#include "DllMacro.h"

namespace Tomahawk
{
    class Playlist;
};

class DLLEXPORT DatabaseCommand_ModifyPlaylist : public DatabaseCommand
{
Q_OBJECT
Q_PROPERTY( int mode READ mode WRITE setMode )

public:
    enum Mode
    {
        ADD = 1,
        REMOVE = 2,
        UPDATE = 3
    };

    explicit DatabaseCommand_ModifyPlaylist( Tomahawk::Playlist* playlist, const QList< Tomahawk::plentry_ptr >& entries, Mode mode );
    virtual ~DatabaseCommand_ModifyPlaylist();

    virtual bool doesMutates() const { return true; }

    virtual void exec( DatabaseImpl* lib );

    int mode() const { return m_mode; }
    void setMode( int m ) { m_mode = (Mode)m; }

private:
    Tomahawk::Playlist* m_playlist;
    QList< Tomahawk::plentry_ptr > m_entries;
    Mode m_mode;
};

#endif // DATABASECOMMAND_MODIFYPLAYLIST_H
