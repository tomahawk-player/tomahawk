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

#include "databasecommand_allalbums.h"

#include <QSqlQuery>

#include "databaseimpl.h"
#include "utils/logger.h"


void
DatabaseCommand_AllAlbums::execForArtist( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::album_ptr> al;
    QString orderToken, sourceToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case ModificationTime:
            orderToken = "file.mtime";
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1 " ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    QString sql = QString(
        "SELECT DISTINCT album.id, album.name "
        "FROM file, file_join "
        "LEFT OUTER JOIN album "
        "ON file_join.album = album.id "
        "WHERE file.id = file_join.file "
        "AND file_join.artist = %1 "
        "%2 "
        "%3 %4 %5"
        ).arg( m_artist->id() )
         .arg( sourceToken )
         .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( orderToken ) : QString() )
         .arg( m_sortDescending ? "DESC" : QString() )
         .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    while( query.next() )
    {
        unsigned int albumId = query.value( 0 ).toUInt();
        QString albumName = query.value( 1 ).toString();
        if ( query.value( 0 ).isNull() )
        {
            albumName = tr( "Unknown" );
        }

        Tomahawk::album_ptr album = Tomahawk::Album::get( albumId, albumName, m_artist );
        al << album;
    }

    emit albums( al, data() );
    emit done();
}


void
DatabaseCommand_AllAlbums::execForCollection( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::album_ptr> al;
    QString orderToken, sourceToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case ModificationTime:
            orderToken = "file.mtime";
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1 " ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    QString sql = QString(
        "SELECT DISTINCT album.id, album.name, album.artist, artist.name "
        "FROM album, file, file_join "
        "LEFT OUTER JOIN artist "
        "ON album.artist = artist.id "
        "WHERE file.id = file_join.file "
        "AND file_join.album = album.id "
        "%1 "
        "%2 %3 %4"
        ).arg( sourceToken )
         .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( orderToken ) : QString() )
         .arg( m_sortDescending ? "DESC" : QString() )
         .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    while( query.next() )
    {
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 2 ).toUInt(), query.value( 3 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( query.value( 0 ).toUInt(), query.value( 1 ).toString(), artist );

        al << album;
    }

    emit albums( al, data() );
    emit done();
}


void
DatabaseCommand_AllAlbums::exec( DatabaseImpl* dbi )
{
    if ( !m_artist.isNull() )
    {
        execForArtist( dbi );
    }
    else
    {
        execForCollection( dbi );
    }
}
