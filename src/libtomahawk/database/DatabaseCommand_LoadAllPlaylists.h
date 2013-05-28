/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DATABASECOMMAND_LOADALLPLAYLIST_H
#define DATABASECOMMAND_LOADALLPLAYLIST_H

// #include <QObject>
// #include <QVariantMap>

// #include "Typedefs.h"
#include "playlist_ptr.h"
#include "DatabaseCommand.h"

#include "DllMacro.h"

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

    explicit DatabaseCommand_LoadAllPlaylists( const Tomahawk::source_ptr& s, QObject* parent = 0 )
        : DatabaseCommand( s, parent )
        , m_limitAmount( 0 )
        , m_sortOrder( None )
        , m_sortDescending( false )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadallplaylists"; }

    void setLimit( unsigned int limit ) { m_limitAmount = limit; }
    void setSortOrder( SortOrder order ) { m_sortOrder = order; }
    void setSortDescending( bool descending ) { m_sortDescending = descending; }

signals:
    void done( const QList<Tomahawk::playlist_ptr>& playlists );

private:
    unsigned int m_limitAmount;
    SortOrder m_sortOrder;
    bool m_sortDescending;
};

#endif // DATABASECOMMAND_LOADALLPLAYLIST_H
