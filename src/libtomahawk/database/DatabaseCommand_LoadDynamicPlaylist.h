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

#ifndef DATABASECOMMAND_LOADDYNAMICPLAYLIST_H
#define DATABASECOMMAND_LOADDYNAMICPLAYLIST_H

#include "DatabaseCommand.h"
// #include "Typedefs.h"
#include "dynplaylist_ptr.h"

namespace Tomahawk
{

class DatabaseCommand_LoadDynamicPlaylist : public DatabaseCommand
{
    Q_OBJECT

public:
    explicit DatabaseCommand_LoadDynamicPlaylist( const source_ptr& s, const QString& guid, QObject* parent = 0 );

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadsingledynamicplaylist"; }

signals:
    void dynamicPlaylistLoaded( const Tomahawk::dynplaylist_ptr& pl );
    void done();

private:
    QString m_plid;
};

}

#endif
