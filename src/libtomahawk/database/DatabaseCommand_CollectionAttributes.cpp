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
#include "DatabaseCommand_CollectionAttributes.h"

#include "DatabaseImpl.h"
#include "Source.h"
#include "database/TomahawkSqlQuery.h"

DatabaseCommand_CollectionAttributes::DatabaseCommand_CollectionAttributes( DatabaseCommand_SetCollectionAttributes::AttributeType type )
    : DatabaseCommand()
    , m_type( type )
{
}

void
DatabaseCommand_CollectionAttributes::exec( DatabaseImpl *lib )
{
    TomahawkSqlQuery query = lib->newquery();

//    QString sourceStr;
//    if ( source().isNull() )
//        sourceStr = "id IS NULL";
//    else
//        sourceStr = QString( "id == %1" ).arg( source()->id() );

    QString typeStr;
    if ( m_type == DatabaseCommand_SetCollectionAttributes::EchonestSongCatalog )
        typeStr = "echonest_song";
    else if ( m_type == DatabaseCommand_SetCollectionAttributes::EchonestArtistCatalog )
        typeStr = "echonest_artist";

    QString queryStr = QString( "SELECT id, v FROM collection_attributes WHERE k = \"%1\"" ).arg( typeStr );
    qDebug() << "Doing queryL" << queryStr;
    query.exec( queryStr );
    PairList data;
    while ( query.next() )
    {
        QPair< QString, QString  > part;
        part.first = query.value( 0 ).toString();
        part.second = query.value( 1 ).toString();
        data << part;
    }
    emit collectionAttributes( data );
}
