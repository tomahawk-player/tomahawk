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

#include "databasecollection.h"

#include "database/database.h"
#include "databasecommand_alltracks.h"
#include "databasecommand_addfiles.h"
#include "databasecommand_deletefiles.h"
#include "databasecommand_loadallplaylists.h"
#include "databasecommand_loadalldynamicplaylists.h"

using namespace Tomahawk;


DatabaseCollection::DatabaseCollection( const source_ptr& src, QObject* parent )
    : Collection( src, QString( "dbcollection:%1" ).arg( src->userName() ), parent )
{
}


void
DatabaseCollection::loadPlaylists()
{
    qDebug() << Q_FUNC_INFO;
    DatabaseCommand_LoadAllPlaylists* cmd = new DatabaseCommand_LoadAllPlaylists( source() );

    connect( cmd,  SIGNAL( done( const QList<Tomahawk::playlist_ptr>& ) ),
                     SLOT( setPlaylists( const QList<Tomahawk::playlist_ptr>& ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::loadDynamicPlaylists()
{
    qDebug() << Q_FUNC_INFO;
    DatabaseCommand_LoadAllDynamicPlaylists* cmd = new DatabaseCommand_LoadAllDynamicPlaylists( source() );
    
    connect( cmd, SIGNAL( playlistLoaded( Tomahawk::source_ptr, QVariantList ) ),
                    SLOT( dynamicPlaylistCreated( const Tomahawk::source_ptr&, const QVariantList& ) ) );
    
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::loadTracks()
{
    qDebug() << Q_FUNC_INFO << source()->userName();

    setLoaded();
    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( source()->collection() );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                    SLOT( setTracks( QList<Tomahawk::query_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::addTracks( const QList<QVariant>& newitems )
{
    qDebug() << Q_FUNC_INFO << newitems.length();
    DatabaseCommand_AddFiles* cmd = new DatabaseCommand_AddFiles( newitems, source() );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::removeTracks( const QDir& dir )
{
    qDebug() << Q_FUNC_INFO << dir;
    DatabaseCommand_DeleteFiles* cmd = new DatabaseCommand_DeleteFiles( dir, source() );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


QList< Tomahawk::playlist_ptr >
DatabaseCollection::playlists()
{
    qDebug() << Q_FUNC_INFO;

    if ( Collection::playlists().isEmpty() )
    {
        loadPlaylists();
    }

    return Collection::playlists();
}


QList< dynplaylist_ptr > DatabaseCollection::dynamicPlaylists()
{
    qDebug() << Q_FUNC_INFO;
    
    if ( Collection::dynamicPlaylists().isEmpty() )
    {
        loadDynamicPlaylists();
    }
    
    return Collection::dynamicPlaylists();
}


QList< Tomahawk::query_ptr >
DatabaseCollection::tracks()
{
    qDebug() << Q_FUNC_INFO;

    if ( !isLoaded() )
    {
        loadTracks();
    }

    return Collection::tracks();
}


void DatabaseCollection::dynamicPlaylistCreated( const source_ptr& source, const QVariantList& data )
{
    dynplaylist_ptr p( new DynamicPlaylist( source,                  //src
                                            data[0].toString(),  //current rev
                                            data[1].toString(),  //title
                                            data[2].toString(),  //info
                                            data[3].toString(),  //creator
                                            data[4].toString(),  // dynamic type
                                            static_cast<GeneratorMode>(data[5].toInt()),  // dynamic mode
                                            data[6].toBool(),    //shared
                                            data[7].toInt(),     //lastmod
                                            data[8].toString() ) );  //GUID
    addDynamicPlaylist( p );
}

