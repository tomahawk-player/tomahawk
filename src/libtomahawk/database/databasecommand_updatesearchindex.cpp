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

#include "databaseimpl.h"
#include "tomahawksqlquery.h"
#include "utils/logger.h"
#include "jobview/IndexingJobItem.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"

#include <QSqlRecord>


DatabaseCommand_UpdateSearchIndex::DatabaseCommand_UpdateSearchIndex()
    : DatabaseCommand()
    , m_statusJob( new IndexingJobItem )
{
    tLog() << Q_FUNC_INFO << "Updating index.";

    JobStatusView::instance()->model()->addJob( m_statusJob );
}


DatabaseCommand_UpdateSearchIndex::~DatabaseCommand_UpdateSearchIndex()
{
    m_statusJob->done();
}


void
DatabaseCommand_UpdateSearchIndex::indexTable( DatabaseImpl* db, const QString& table, const QString& query )
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSqlQuery q = db->newquery();
    qDebug() << "Building index for" << table;
    q.exec( QString( "SELECT %1" ).arg( query ) );

    QMap< unsigned int, QString > fields;
    QString value;
    while ( q.next() )
    {
        value = "";
        for ( int v = 1; v < q.record().count(); v++ )
            value += q.value( v ).toString() + " ";

        fields.insert( q.value( 0 ).toUInt(), value.trimmed() );
    }

    db->m_fuzzyIndex->appendFields( table, fields );
    qDebug() << "Building index for" << table << "finished.";
}


void
DatabaseCommand_UpdateSearchIndex::exec( DatabaseImpl* db )
{
    db->m_fuzzyIndex->beginIndexing();

    indexTable( db, "artist", "id, name FROM artist" );
    indexTable( db, "album", "id, name FROM album" );
    indexTable( db, "track", "id, name FROM track" );
    indexTable( db, "trackartist", "track.id, artist.name, track.name FROM track, artist WHERE artist.id = track.artist" );

    db->m_fuzzyIndex->endIndexing();
}
