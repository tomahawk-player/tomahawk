#include "result.h"

#include "album.h"
#include "collection.h"

using namespace Tomahawk;

Result::Result() {}

Result::Result( const QVariant& v, const collection_ptr& collection )
    : QObject()
    , m_v( v )
    , m_collection( collection )
{
    QVariantMap m = m_v.toMap();

    m_artist = Artist::get( m.value( "artistid" ).toUInt(), m.value( "artist" ).toString(), collection );
    m_album = Album::get( m.value( "albumid" ).toUInt(), m.value( "album" ).toString(), m_artist, collection );
    m_track = m.value( "track" ).toString();
    m_url = m.value( "url" ).toString();
    m_mimetype = m.value( "mimetype" ).toString();

    m_duration = m.value( "duration" ).toUInt();
    m_bitrate = m.value( "bitrate" ).toUInt();
    m_size = m.value( "size" ).toUInt();
    m_albumpos = m.value( "albumpos" ).toUInt();
    m_modtime = m.value( "mtime" ).toUInt();
    m_score = m.value( "score" ).toFloat();
    m_year = 0;

    m_id = m.value( "id" ).toUInt();

    if ( !m_collection.isNull() )
    {
        connect( m_collection->source().data(), SIGNAL( online() ), SLOT( onOnline() ), Qt::QueuedConnection );
        connect( m_collection->source().data(), SIGNAL( offline() ), SLOT( onOffline() ), Qt::QueuedConnection );
    }
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
    if ( m_rid.isEmpty() )
    {
        m_rid = m_v.toMap().value( "sid" ).toString();
    }
    return m_rid;
}


QString
Result::toString() const
{
    return QString( "Result(%1 %2\t%3 - %4  %5" ).arg( id() ).arg( score() ).arg( artist()->name() ).arg( track() ).arg( url() );
}


Tomahawk::query_ptr
Result::toQuery() const
{
    Tomahawk::query_ptr query = Tomahawk::Query::get( toVariant(), false );
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
