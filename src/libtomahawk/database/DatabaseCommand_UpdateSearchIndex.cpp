/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
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

#include "DatabaseCommand_UpdateSearchIndex.h"

#include <QSqlRecord>

#include "DatabaseImpl.h"
#include "Source.h"
#include "TomahawkSqlQuery.h"
#include "jobview/IndexingJobItem.h"

#ifndef ENABLE_HEADLESS
    #include "jobview/JobStatusView.h"
    #include "jobview/JobStatusModel.h"
#endif

#include "utils/Logger.h"


DatabaseCommand_UpdateSearchIndex::DatabaseCommand_UpdateSearchIndex()
    : DatabaseCommand()
    , m_statusJob( new IndexingJobItem )
{
    tLog() << Q_FUNC_INFO << "Updating index.";

#ifndef ENABLE_HEADLESS
    JobStatusView::instance()->model()->addJob( m_statusJob.data() );
#endif
}


DatabaseCommand_UpdateSearchIndex::~DatabaseCommand_UpdateSearchIndex()
{
#ifndef ENABLE_HEADLESS
    if ( ! m_statusJob.isNull() )
        m_statusJob.data()->done();
#endif
}


void
DatabaseCommand_UpdateSearchIndex::exec( DatabaseImpl* db )
{
    db->m_fuzzyIndex->beginIndexing();

    QMap< unsigned int, QMap< QString, QString > > data;
    TomahawkSqlQuery q = db->newquery();

    q.exec( "SELECT track.id, track.name, artist.name, artist.id FROM track, artist WHERE artist.id = track.artist" );
    while ( q.next() )
    {
        QMap< QString, QString > track;
        track.insert( "track", q.value( 1 ).toString() );
        track.insert( "artist", q.value( 2 ).toString() );
        track.insert( "artistid", q.value( 3 ).toString() );

        data.insert( q.value( 0 ).toUInt(), track );
    }

    db->m_fuzzyIndex->appendFields( data );
    data.clear();

    q.exec( "SELECT album.id, album.name FROM album" );
    while ( q.next() )
    {
        QMap< QString, QString > album;
        album.insert( "album", q.value( 1 ).toString() );

        data.insert( q.value( 0 ).toUInt(), album );
    }

    db->m_fuzzyIndex->appendFields( data );

    qDebug() << "Building index finished.";

    db->m_fuzzyIndex->endIndexing();
}
