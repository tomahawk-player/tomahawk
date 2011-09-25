/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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
#include "DatabaseCommand_SetCollectionAttributes.h"

#include "databaseimpl.h"
#include "source.h"

DatabaseCommand_SetCollectionAttributes::DatabaseCommand_SetCollectionAttributes( const Tomahawk::source_ptr& source, AttributeType type, const QByteArray& id )
    : DatabaseCommandLoggable( source )
    , m_id( id )
    , m_type( type )
{
}

void
DatabaseCommand_SetCollectionAttributes::exec( DatabaseImpl *lib )
{
    TomahawkSqlQuery query = lib->newquery();

    QString sourceStr;
    if ( source().isNull() || source()->isLocal() )
        sourceStr = "NULL";
    else
        sourceStr = QString( "%1" ).arg( source()->id() );

    QString typeStr;
    if ( m_type == EchonestSongCatalog )
        typeStr = "echonest_song";
    else if ( m_type == EchonestArtistCatalog )
        typeStr = "echonest_artist";

    QString queryStr = QString( "REPLACE INTO collection_attributes ( id, k, v ) VALUES( %1, \"%2\", \"%3\" )" ).arg( sourceStr ).arg( typeStr ).arg( QString::fromUtf8( m_id ) );
    qDebug() << "Doing query:" << queryStr;
    query.exec( queryStr );
}
