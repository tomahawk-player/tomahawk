/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "ScriptCommand_AllTracks.h"


#include "ScriptAccount.h"
#include "PlaylistEntry.h"
#include "ScriptCollection.h"
#include "Artist.h"
#include "Album.h"
#include "ScriptJob.h"
#include "utils/Logger.h"
#include "../Result.h"

#include <QtConcurrentRun>

using namespace Tomahawk;

ScriptCommand_AllTracks::ScriptCommand_AllTracks( const Tomahawk::collection_ptr& collection,
                                                  const Tomahawk::album_ptr& album,
                                                  QObject* parent )
    : ScriptCommand( parent )
    , m_collection( collection )
    , m_album( album )
{
}


void
ScriptCommand_AllTracks::enqueue()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        emit tracks( QList< Tomahawk::query_ptr >() );
        return;
    }

    exec();
}


void
ScriptCommand_AllTracks::exec()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        reportFailure();
        return;
    }

    ScriptJob* job;

    if ( m_album )
    {
        QVariantMap arguments;
        arguments[ "artist" ] = m_album->artist()->name();
        arguments[ "album" ] = m_album->name();

        job = collection->scriptObject()->invoke( "albumTracks", arguments );
    }
    else
    {
        job = collection->scriptObject()->invoke( "tracks" );
    }


    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( onTracksJobDone( QVariantMap ) ), Qt::QueuedConnection );
    job->start();
}


void
ScriptCommand_AllTracks::reportFailure()
{
    if ( m_album && m_collection )
        tDebug() << Q_FUNC_INFO << "for collection" << m_collection->name() << "artist" << m_album->artist()->name() << "album" << m_album->name();
    else if ( m_collection )
        tDebug() << Q_FUNC_INFO << "for collection" << m_collection->name() << "(no more information available)";
    else
        tDebug() << Q_FUNC_INFO;

    emit tracks( QList< Tomahawk::query_ptr >() );
    emit done();
}


void
ScriptCommand_AllTracks::onTracksJobDone( const QVariantMap& result )
{
    ScriptJob* job = qobject_cast< ScriptJob* >( sender() );
    Q_ASSERT( job );

    //qDebug() << "Resolver reporting album tracks:" << result;

    if ( job->error() )
    {
        reportFailure();
        return;
    }

    QSharedPointer< ScriptCollection > collection = m_collection.objectCast< ScriptCollection >();
    Q_ASSERT( !collection.isNull() );

    QtConcurrent::run( [] ( ScriptCommand_AllTracks* t, ScriptJob* job, const QVariantMap& result, const QSharedPointer< ScriptCollection >& collection )
    {
        QList< Tomahawk::result_ptr > results = collection->scriptAccount()->parseResultVariantList( result[ "tracks" ].toList() );

        QList< Tomahawk::query_ptr > queries;
        foreach ( const Tomahawk::result_ptr& result, results )
        {
            result->setResolvedByCollection( collection );
            queries.append( result->toQuery() );
        }

        tDebug() << Q_FUNC_INFO << "about to push" << queries.count() << "tracks";

        emit t->tracks( queries );
        emit t->done();

        job->deleteLater();
    }, this, job, result, collection );
}
