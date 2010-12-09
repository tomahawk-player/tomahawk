/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
#ifndef DATABASECOMMAND_LOADALLDYNAMICPLAYLISTS_H
#define DATABASECOMMAND_LOADALLDYNAMICPLAYLISTS_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_LoadAllDynamicPlaylists : public DatabaseCommand
{
    Q_OBJECT
    
public:
    explicit DatabaseCommand_LoadAllDynamicPlaylists( const Tomahawk::source_ptr& s, QObject* parent = 0 )
    : DatabaseCommand( s, parent )
    {}
    
    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadalldynamicplaylists"; }
    
signals:
    void done( const QList<Tomahawk::dynplaylist_ptr>& playlists );
};

#endif // DATABASECOMMAND_ADDFILES_H
