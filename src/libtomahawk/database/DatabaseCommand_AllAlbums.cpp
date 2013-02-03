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

#include "DatabaseCommand_AllAlbums.h"

#include <QSqlQuery>

#include "Artist.h"
#include "DatabaseImpl.h"
#include "Source.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


DatabaseCommand_AllAlbums::DatabaseCommand_AllAlbums( const Tomahawk::collection_ptr &collection, const Tomahawk::artist_ptr &artist, QObject *parent )
  : DatabaseCommand( parent )
  , m_collection( collection )
  , m_artist( artist )
  , m_amount( 0 )
  , m_sortOrder( DatabaseCommand_AllAlbums::None )
  , m_sortDescending( false )
{
}


DatabaseCommand_AllAlbums::~DatabaseCommand_AllAlbums()
{}


void
DatabaseCommand_AllAlbums::setArtist( const Tomahawk::artist_ptr &artist )
{
    m_artist = artist;
}


void
DatabaseCommand_AllAlbums::execForArtist( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::album_ptr> al;
    QString orderToken, sourceToken, filterToken, tables;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case ModificationTime:
            orderToken = "file.mtime";
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1 " ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    if ( !m_filter.isEmpty() )
    {
        QString filtersql;
        QStringList sl = m_filter.split( " ", QString::SkipEmptyParts );
        foreach( QString s, sl )
        {
            filtersql += QString( " AND ( artist.name LIKE '%%1%' OR album.name LIKE '%%1%' OR track.name LIKE '%%1%' )" ).arg( TomahawkSqlQuery::escape( s ) );
        }

        filterToken = QString( "AND artist.id = file_join.artist AND file_join.track = track.id %1" ).arg( filtersql );
        tables = "file, file_join, artist, track";
    }
    else
        tables = "file, file_join";

    QString sql = QString(
        "SELECT DISTINCT album.id, album.name "
        "FROM %1 "
        "LEFT OUTER JOIN album ON file_join.album = album.id "
        "WHERE file.id = file_join.file "
        "AND file_join.artist = %2 "
        "%3 %4 %5 %6 %7"
        ).arg( tables )
         .arg( m_artist->id() )
         .arg( sourceToken )
         .arg( filterToken )
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
    emit albums( al );
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
        "FROM file_join, file, album "
        "LEFT OUTER JOIN artist ON album.artist = artist.id "
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
    emit albums( al );
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
