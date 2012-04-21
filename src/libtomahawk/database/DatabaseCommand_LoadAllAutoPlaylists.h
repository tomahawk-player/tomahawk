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

#ifndef DATABASECOMMAND_LOADALLAUTOPLAYLISTS_H
#define DATABASECOMMAND_LOADALLAUTOPLAYLISTS_H

#include <QObject>
#include <QVariantMap>

#include "DatabaseCommand.h"
#include "Typedefs.h"
#include "DatabaseCommand_LoadAllPlaylists.h"

class DatabaseCommand_LoadAllAutoPlaylists : public DatabaseCommand
{
    Q_OBJECT

public:
    explicit DatabaseCommand_LoadAllAutoPlaylists( const Tomahawk::source_ptr& s, QObject* parent = 0 )
    : DatabaseCommand( s, parent )
    , m_limitAmount( 0 )
    , m_sortOrder( DatabaseCommand_LoadAllPlaylists::None )
    , m_sortDescending( false )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadallautoplaylists"; }

    void setLimit( unsigned int limit ) { m_limitAmount = limit; }
    void setSortOrder( DatabaseCommand_LoadAllPlaylists::SortOrder order ) { m_sortOrder = order; }
    void setSortDescending( bool descending ) { m_sortDescending = descending; }

signals:
    void autoPlaylistLoaded( const Tomahawk::source_ptr& source, const QVariantList& data );
    void done();

private:
    unsigned int m_limitAmount;
    DatabaseCommand_LoadAllPlaylists::SortOrder m_sortOrder;
    bool m_sortDescending;
};

#endif
