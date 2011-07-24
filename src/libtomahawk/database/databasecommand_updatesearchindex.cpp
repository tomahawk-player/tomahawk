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

#include "databasecommand_updatesearchindex.h"

#include "utils/logger.h"


DatabaseCommand_UpdateSearchIndex::DatabaseCommand_UpdateSearchIndex()
    : DatabaseCommand()
{
}


void
DatabaseCommand_UpdateSearchIndex::indexTable( DatabaseImpl* db, const QString& table )
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSqlQuery query = db->newquery();
    qDebug() << "Building index for" << table;
    query.exec( QString( "SELECT id, name FROM %1" ).arg( table ) );

    QMap< unsigned int, QString > fields;
    while ( query.next() )
    {
        fields.insert( query.value( 0 ).toUInt(), query.value( 1 ).toString() );
    }

    db->m_fuzzyIndex->appendFields( table, fields );
    qDebug() << "Building index for" << table << "finished.";
}


void
DatabaseCommand_UpdateSearchIndex::exec( DatabaseImpl* db )
{
    db->m_fuzzyIndex->beginIndexing();

    indexTable( db, "artist" );
    indexTable( db, "album" );
    indexTable( db, "track" );

    db->m_fuzzyIndex->endIndexing();
}
