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

#include "databasecommand_createdynamicplaylist.h"

#include <QSqlQuery>
#include <QSqlDriver>

#include "dynamic/DynamicPlaylist.h"
#include "dynamic/DynamicControl.h"
#include "dynamic/GeneratorInterface.h"

#include "network/servent.h"
#include "playlist/playlistmanager.h"

using namespace Tomahawk;


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( QObject* parent )
: DatabaseCommand_CreatePlaylist( parent )
{
    qDebug() << Q_FUNC_INFO << "creating dynamiccreatecommand 1";
}


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( const source_ptr& author,
                                                                const dynplaylist_ptr& playlist )
    : DatabaseCommand_CreatePlaylist( author, playlist.staticCast<Playlist>() )
    , m_playlist( playlist )
{
    qDebug() << Q_FUNC_INFO << "creating dynamiccreatecommand 2";
}


void
DatabaseCommand_CreateDynamicPlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !( m_playlist.isNull() && m_v.isNull() ) );
    Q_ASSERT( !source().isNull() );

    DatabaseCommand_CreatePlaylist::createPlaylist( lib, true );
    qDebug() << "Created normal playlist, now creating additional dynamic info!";

    TomahawkSqlQuery cre = lib->newquery();

    cre.prepare( "INSERT INTO dynamic_playlist( guid, pltype, plmode ) "
                 "VALUES( ?, ?, ? )" );

    if( m_playlist.isNull() ) {
        QVariantMap m = m_v.toMap();
        cre.addBindValue( m.value( "guid" ) );
        cre.addBindValue( m.value( "type" ) );
        cre.addBindValue( m.value( "mode" ) );
    } else {
        cre.addBindValue( m_playlist->guid() );
        cre.addBindValue( m_playlist->type() );
        cre.addBindValue( m_playlist->mode() );
    }
    cre.exec();
}


void
DatabaseCommand_CreateDynamicPlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    if ( source().isNull() || source()->collection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }

    if( report() == false )
        return;

    qDebug() << Q_FUNC_INFO << "..reporting..";
    if( m_playlist.isNull() ) {
        source_ptr src = source();
        QMetaObject::invokeMethod( PlaylistManager::instance(),
                                   "createDynamicPlaylist",
                                   Qt::BlockingQueuedConnection,
                                   QGenericArgument( "Tomahawk::source_ptr", (const void*)&src ),
                                   Q_ARG( QVariant, m_v ) );
    } else {
        m_playlist->reportCreated( m_playlist );
    }
    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
