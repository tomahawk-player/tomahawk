/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi            <lfranchi@kde.org>
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

#include "DatabaseCollection.h"

#include "database/Database.h"
#include "DatabaseCommand_AllTracks.h"
#include "DatabaseCommand_AddFiles.h"
#include "DatabaseCommand_DeleteFiles.h"
#include "DatabaseCommand_LoadAllPlaylists.h"
#include "DatabaseCommand_LoadAllAutoPlaylists.h"
#include "DatabaseCommand_LoadAllStations.h"

#include "utils/Logger.h"

using namespace Tomahawk;


DatabaseCollection::DatabaseCollection( const source_ptr& src, QObject* parent )
    : Collection( src, QString( "dbcollection:%1" ).arg( src->userName() ), parent )
{
}


void
DatabaseCollection::loadPlaylists()
{
    DatabaseCommand_LoadAllPlaylists* cmd = new DatabaseCommand_LoadAllPlaylists( source() );

    connect( cmd,  SIGNAL( done( const QList<Tomahawk::playlist_ptr>& ) ),
                     SLOT( setPlaylists( const QList<Tomahawk::playlist_ptr>& ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::loadAutoPlaylists()
{
    DatabaseCommand_LoadAllAutoPlaylists* cmd = new DatabaseCommand_LoadAllAutoPlaylists( source() );

    connect( cmd, SIGNAL( autoPlaylistLoaded( Tomahawk::source_ptr, QVariantList ) ),
                    SLOT( autoPlaylistCreated( const Tomahawk::source_ptr&, const QVariantList& ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::loadStations()
{
    DatabaseCommand_LoadAllStations* cmd = new DatabaseCommand_LoadAllStations( source() );

    connect( cmd, SIGNAL( stationLoaded( Tomahawk::source_ptr, QVariantList ) ),
             SLOT( stationCreated( const Tomahawk::source_ptr&, const QVariantList& ) ) );

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
    if ( Collection::playlists().isEmpty() )
    {
        loadPlaylists();
    }

    return Collection::playlists();
}


QList< dynplaylist_ptr >
DatabaseCollection::autoPlaylists()
{
    if ( Collection::autoPlaylists().isEmpty() )
    {
        loadAutoPlaylists();
    }

    return Collection::autoPlaylists();
}


QList< dynplaylist_ptr >
DatabaseCollection::stations()
{
    if ( Collection::stations().isEmpty() )
    {
        loadStations();
    }

    return Collection::stations();
}


void
DatabaseCollection::autoPlaylistCreated( const source_ptr& source, const QVariantList& data )
{
    dynplaylist_ptr p( new DynamicPlaylist( source,                  //src
                                            data[0].toString(),  //current rev
                                            data[1].toString(),  //title
                                            data[2].toString(),  //info
                                            data[3].toString(),  //creator
                                            data[4].toUInt(),  // createdOn
                                            data[5].toString(),  // dynamic type
                                            static_cast<GeneratorMode>(data[6].toInt()),  // dynamic mode
                                            data[7].toBool(),    //shared
                                            data[8].toInt(),     //lastmod
                                            data[9].toString() ), &QObject::deleteLater );  //GUID
    addAutoPlaylist( p );
}


void
DatabaseCollection::stationCreated( const source_ptr& source, const QVariantList& data )
{
    dynplaylist_ptr p( new DynamicPlaylist( source,                  //src
                                            data[0].toString(),  //current rev
                                            data[1].toString(),  //title
                                            data[2].toString(),  //info
                                            data[3].toString(),  //creator
                                            data[4].toUInt(),  // createdOn
                                            data[5].toString(),  // dynamic type
                                            static_cast<GeneratorMode>(data[6].toInt()),  // dynamic mode
                                            data[7].toBool(),    //shared
                                            data[8].toInt(),     //lastmod
                                            data[9].toString() ), &QObject::deleteLater );  //GUID
    addStation( p );
}


