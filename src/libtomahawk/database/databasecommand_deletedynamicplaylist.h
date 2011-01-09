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

#ifndef DATABASECOMMAND_DELETEDYNAMICPLAYLIST_H
#define DATABASECOMMAND_DELETEDYNAMICPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommand_deleteplaylist.h"
#include "source.h"
#include "typedefs.h"

class DatabaseCommand_DeleteDynamicPlaylist : public DatabaseCommand_DeletePlaylist
{
    Q_OBJECT    
public:
    explicit DatabaseCommand_DeleteDynamicPlaylist( QObject* parent = 0 )
    : DatabaseCommand_DeletePlaylist( parent )
    {}
    
    explicit DatabaseCommand_DeleteDynamicPlaylist( const Tomahawk::source_ptr& source, const QString& playlistguid );
    
    QString commandname() const { return "deletedynamicplaylist"; }
    
    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }
   
};

#endif // DATABASECOMMAND_DELETEDYNAMICPLAYLIST_H
