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

#include "ExternalResolver.h"
#include "ScriptCollection.h"

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

    collection->resolver()->enqueue( QSharedPointer< ScriptCommand >( this ) );
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

    if ( m_album.isNull() )
    {
        reportFailure();
        return;
    }

    connect( collection->resolver(), SIGNAL( tracksFound( QList< Tomahawk::query_ptr > ) ),
             this, SLOT( onResolverDone( QList< Tomahawk::query_ptr > ) ) );

    collection->resolver()->tracks( m_collection, m_album );
}


void
ScriptCommand_AllTracks::reportFailure()
{
    emit tracks( QList< Tomahawk::query_ptr >() );
    emit done();
}


void
ScriptCommand_AllTracks::onResolverDone( const QList< Tomahawk::query_ptr >& q )
{
    emit tracks( q );
    emit done();
}
