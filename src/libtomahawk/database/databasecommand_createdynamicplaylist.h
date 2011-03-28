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

#ifndef DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
#define DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommand_createplaylist.h"
#include "dynamic/DynamicPlaylist.h"
#include "typedefs.h"

class DatabaseCommand_CreateDynamicPlaylist : public DatabaseCommand_CreatePlaylist
{
    Q_OBJECT
    Q_PROPERTY( QVariant playlist READ playlistV WRITE setPlaylistV )
    
public:
    explicit DatabaseCommand_CreateDynamicPlaylist( QObject* parent = 0 );
    explicit DatabaseCommand_CreateDynamicPlaylist( const Tomahawk::source_ptr& author, const Tomahawk::dynplaylist_ptr& playlist );
    
    QString commandname() const { return "createdynamicplaylist"; }
    
    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }
    
    QVariant playlistV() const
    {
        if( m_v.isNull() )
            return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
        else
            return m_v;
    }
    
    void setPlaylistV( const QVariant& v )
    {
        m_v = v;
    }
    
private:
    Tomahawk::dynplaylist_ptr m_playlist;
};

#endif // DATABASECOMMAND_CREATEDYNAMICPLAYLIST_H
