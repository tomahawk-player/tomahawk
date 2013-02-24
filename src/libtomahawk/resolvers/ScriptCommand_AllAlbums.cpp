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

#include "ScriptCommand_AllAlbums.h"

#include "ExternalResolver.h"
#include "ScriptCollection.h"
#include "Album.h"
#include "Artist.h"

#include "utils/Logger.h"

ScriptCommand_AllAlbums::ScriptCommand_AllAlbums( const Tomahawk::collection_ptr& collection,
                                                  const Tomahawk::artist_ptr& artist,
                                                  QObject* parent )
    : ScriptCommand( parent )
    , m_collection( collection )
    , m_artist( artist )
{
}


void
ScriptCommand_AllAlbums::enqueue()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        emit albums( QList< Tomahawk::album_ptr >() );
        return;
    }

    collection->resolver()->enqueue( QSharedPointer< ScriptCommand >( this ) );
}


void
ScriptCommand_AllAlbums::setFilter( const QString& filter )
{
    m_filter = filter;
}


void
ScriptCommand_AllAlbums::exec()
{
    Tomahawk::ScriptCollection* collection = qobject_cast< Tomahawk::ScriptCollection* >( m_collection.data() );
    if ( collection == 0 )
    {
        reportFailure();
        return;
    }

    if ( m_artist.isNull() )
    {
        reportFailure();
        return;
    }

    connect( collection->resolver(), SIGNAL( albumsFound( QList< Tomahawk::album_ptr > ) ),
             this, SLOT( onResolverDone( QList< Tomahawk::album_ptr > ) ) );

    collection->resolver()->albums( m_collection, m_artist );
}


void
ScriptCommand_AllAlbums::reportFailure()
{
    emit albums( QList< Tomahawk::album_ptr >() );
    emit done();
}


void
ScriptCommand_AllAlbums::onResolverDone( const QList< Tomahawk::album_ptr >& a )
{
    if ( m_filter.isEmpty() )
        emit albums( a );
    else
    {
        QList< Tomahawk::album_ptr > filtered;
        foreach( const Tomahawk::album_ptr& album, a )
        {
            if( album->name().toLower().contains( m_filter.toLower() ) ||
                album->artist()->name().toLower().contains( m_filter.toLower() ) )
                filtered.append( album );
        }
        emit albums( filtered );
    }
    emit done();
}
