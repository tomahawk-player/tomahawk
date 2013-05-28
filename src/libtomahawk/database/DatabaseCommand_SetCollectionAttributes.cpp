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

#include "DatabaseImpl.h"
#include "Source.h"
#include "network/Servent.h"
#include "SourceList.h"
#include "EchonestCatalogSynchronizer.h"
#include "database/TomahawkSqlQuery.h"

DatabaseCommand_SetCollectionAttributes::DatabaseCommand_SetCollectionAttributes( AttributeType type, const QByteArray& id )
    : DatabaseCommandLoggable( )
    , m_delete( false )
    , m_type( type )
    , m_id( id )
{
}

DatabaseCommand_SetCollectionAttributes::DatabaseCommand_SetCollectionAttributes( DatabaseCommand_SetCollectionAttributes::AttributeType type, bool toDelete )
    : DatabaseCommandLoggable()
    , m_delete( toDelete )
    , m_type( type )
{
}


void
DatabaseCommand_SetCollectionAttributes::exec( DatabaseImpl *lib )
{
    TomahawkSqlQuery query = lib->newquery();

    QString sourceStr;
    if ( source().isNull() )
        setSource( SourceList::instance()->getLocal() );

    if ( source().isNull() || source()->isLocal() )
        sourceStr = "NULL";
    else
        sourceStr = QString( "%1" ).arg( source()->id() );

    QString typeStr;
    if ( m_type == EchonestSongCatalog )
        typeStr = "echonest_song";
    else if ( m_type == EchonestArtistCatalog )
        typeStr = "echonest_artist";

    TomahawkSqlQuery delQuery = lib->newquery();
    delQuery.exec( QString( "DELETE FROM collection_attributes WHERE id %1" ).arg( source()->isLocal() ? QString("IS NULL") : QString( "= %1" ).arg( source()->id() )));

    if ( m_delete )
        return;

    QString queryStr = QString( "INSERT INTO collection_attributes ( id, k, v ) VALUES( %1, \"%2\", \"%3\" )" ).arg( sourceStr ).arg( typeStr ).arg( QString::fromUtf8( m_id ) );
    qDebug() << "Doing query:" << queryStr;
    query.exec( queryStr );
}

void
DatabaseCommand_SetCollectionAttributes::postCommitHook()
{
    if ( m_type == EchonestSongCatalog ||
         m_type == EchonestArtistCatalog )
        Tomahawk::EchonestCatalogSynchronizer::instance()->knownCatalogsChanged();

    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
