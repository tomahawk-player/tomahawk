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
#include "databasecommand_deletedynamicplaylist.h"

#include <QSqlQuery>
#include "network/servent.h"
	
using namespace Tomahawk;


DatabaseCommand_DeleteDynamicPlaylist::DatabaseCommand_DeleteDynamicPlaylist( const source_ptr& source, const QString& playlistguid )
    : DatabaseCommand_DeletePlaylist( source, playlistguid )
{
}


void
DatabaseCommand_DeleteDynamicPlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "deleting dynamic playlist:" << m_playlistguid;
    DatabaseCommand_DeletePlaylist::exec( lib );
    TomahawkSqlQuery cre = lib->newquery();
    
    cre.prepare( "DELETE FROM dynamic_playlist WHERE guid = :id" );
    cre.bindValue( ":id", m_playlistguid );
    
    cre.exec();
}


void
DatabaseCommand_DeleteDynamicPlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO << "..reporting..:" << m_playlistguid;
    if ( source().isNull() || source()->collection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }
    
    dynplaylist_ptr playlist = source()->collection()->dynamicPlaylist( m_playlistguid );
    Q_ASSERT( !playlist.isNull() );
    
    playlist->reportDeleted( playlist );
    
    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
