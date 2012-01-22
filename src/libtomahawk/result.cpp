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

#include "result.h"

#include "album.h"
#include "collection.h"
#include "source.h"
#include "database/database.h"
#include "database/databasecommand_resolve.h"
#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_addfiles.h"

#include "utils/logger.h"

using namespace Tomahawk;

static QHash< QString, QWeakPointer< Result > > s_results;
static QMutex s_mutex;


Tomahawk::result_ptr
Result::get( const QString& url )
{
    QMutexLocker lock( &s_mutex );
    if ( s_results.contains( url ) )
    {
        return s_results.value( url );
    }

    result_ptr r = result_ptr( new Result( url ) );
    s_results.insert( url, r );

    return r;
}


Result::Result( const QString& url )
    : QObject()
    , m_url( url )
    , m_duration( 0 )
    , m_bitrate( 0 )
    , m_size( 0 )
    , m_albumpos( 0 )
    , m_modtime( 0 )
    , m_year( 0 )
    , m_score( 0 )
    , m_trackId( 0 )
    , m_fileId( 0 )
{
}


Result::~Result()
{
    QMutexLocker lock( &s_mutex );
    if ( s_results.contains( m_url ) )
    {
        s_results.remove( m_url );
    }
}


artist_ptr
Result::artist() const
{
    return m_artist;
}


album_ptr
Result::album() const
{
    return m_album;
}


collection_ptr
Result::collection() const
{
    return m_collection;
}


float
Result::score() const
{
    if ( !collection().isNull() && collection()->source()->isOnline() )
    {
        return m_score;
    }
    else
    {
        // check if this a valid collection-less result (e.g. from youtube, but ignore offline sources still)
        if ( collection().isNull() )
            return m_score;
        else
            return 0.0;
    }
}


RID
Result::id() const
{
    if ( m_rid.isEmpty() )
        m_rid = uuid();

    return m_rid;
}


bool
Result::isOnline() const
{
    return ( ( !collection().isNull() && collection()->source()->isOnline() ) || collection().isNull() );
}


QVariant
Result::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", artist()->name() );
    m.insert( "album", album()->name() );
    m.insert( "track", track() );

    if ( !collection().isNull() )
        m.insert( "source", collection()->source()->friendlyName() );
    else
        m.insert( "source", friendlySource() );

    m.insert( "mimetype", mimetype() );
    m.insert( "size", size() );
    m.insert( "bitrate", bitrate() );
    m.insert( "duration", duration() );
    m.insert( "score", score() );
    m.insert( "sid", id() );

    return m;
}


QString
Result::toString() const
{
    return QString( "Result(%1 %2\t%3 - %4  %5" ).arg( id() ).arg( score() ).arg( artist()->name() ).arg( track() ).arg( url() );
}


Tomahawk::query_ptr
Result::toQuery()
{
    if ( m_query.isNull() )
    {
        m_query = Tomahawk::Query::get( artist()->name(), track(), album()->name() );
        QList<Tomahawk::result_ptr> rl;
        rl << Result::get( m_url );

        m_query->addResults( rl );
        m_query->setResolveFinished( true );
    }

    return m_query;
}


void
Result::updateAttributes()
{
    if ( m_attributes.contains( "releaseyear" ) )
    {
        m_year = m_attributes.value( "releaseyear" ).toInt();
    }
}


void
Result::onOnline()
{
    emit statusChanged();
}


void
Result::onOffline()
{
    emit statusChanged();
}


void
Result::setArtist( const Tomahawk::artist_ptr& artist )
{
    m_artist = artist;
}


void
Result::setAlbum( const Tomahawk::album_ptr& album )
{
    m_album = album;
}


void
Result::setCollection( const Tomahawk::collection_ptr& collection )
{
    m_collection = collection;
    connect( m_collection->source().data(), SIGNAL( online() ), SLOT( onOnline() ), Qt::QueuedConnection );
    connect( m_collection->source().data(), SIGNAL( offline() ), SLOT( onOffline() ), Qt::QueuedConnection );
}


QString
Result::friendlySource() const
{
    if ( collection().isNull() )
    {
        return m_friendlySource;
    }
    else
        return collection()->source()->friendlyName();
}
