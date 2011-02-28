#include "result.h"

#include "album.h"
#include "collection.h"
#include "database/databasecommand_resolve.h"
#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_addfiles.h"
#include "database/databasecommand_loadfile.h"

using namespace Tomahawk;


Result::Result()
    : QObject()
    , m_year( 0 )
{
}


Result::~Result()
{
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
    if ( collection()->source()->isOnline() )
    {
        return m_score;
    }
    else
        return 0.0;
}


RID
Result::id() const
{
    Q_ASSERT( !m_rid.isEmpty() );
    return m_rid;
}


QVariant
Result::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", artist()->name() );
    m.insert( "album", album()->name() );
    m.insert( "track", track() );
    m.insert( "source", collection()->source()->friendlyName() );
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
Result::toQuery() const
{
    Tomahawk::query_ptr query = Tomahawk::Query::get( artist()->name(), track(), album()->name() );
    return query;
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
//    qDebug() << Q_FUNC_INFO << toString();
    emit statusChanged();
}


void
Result::onOffline()
{
//    qDebug() << Q_FUNC_INFO << toString();
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
