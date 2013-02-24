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

#include "ScriptCommand_AllArtists.h"

#include "Artist.h"
#include "ExternalResolver.h"
#include "ScriptCollection.h"

ScriptCommand_AllArtists::ScriptCommand_AllArtists( const Tomahawk::collection_ptr& collection,
                                                    QObject* parent )
    : ScriptCommand( parent )
    , m_collection( collection )
{
}


void
ScriptCommand_AllArtists::enqueue()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        reportFailure();
        return;
    }

    collection->resolver()->enqueue( QSharedPointer< ScriptCommand >( this ) );
}


void
ScriptCommand_AllArtists::setFilter( const QString& filter )
{
    m_filter = filter;
}


void
ScriptCommand_AllArtists::exec()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        emit artists( QList< Tomahawk::artist_ptr >() );
        return;
    }

    connect( collection->resolver(), SIGNAL( artistsFound( QList< Tomahawk::artist_ptr > ) ),
             this, SLOT( onResolverDone( QList< Tomahawk::artist_ptr > ) ) );

    collection->resolver()->artists( m_collection );
}


void
ScriptCommand_AllArtists::reportFailure()
{
    emit artists( QList< Tomahawk::artist_ptr >() );
    emit done();
}


void ScriptCommand_AllArtists::onResolverDone( const QList< Tomahawk::artist_ptr >& a )
{
    if ( m_filter.isEmpty() )
        emit artists( a );
    else
    {
        QList< Tomahawk::artist_ptr > filtered;
        foreach( const Tomahawk::artist_ptr& artist, a )
        {
            if ( artist->name().contains( m_filter ) )
                filtered.append( artist );
        }
        emit artists( filtered );
    }
    emit done();
}
