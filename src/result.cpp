#include "tomahawk/result.h"

using namespace Tomahawk;


Result::Result( const QVariant& v, const collection_ptr& collection )
    : m_v( v )
    , m_collection( collection )
{
    QVariantMap m = m_v.toMap();

    m_artist = m.value( "artist" ).toString();
    m_album = m.value( "album" ).toString();
    m_track = m.value( "track" ).toString();
    m_url = m.value( "url" ).toString();
    m_mimetype = m.value( "mimetype" ).toString();

    m_duration = m.value( "duration" ).toUInt();
    m_bitrate = m.value( "bitrate" ).toUInt();
    m_size = m.value( "size" ).toUInt();
    m_albumpos = m.value( "albumpos" ).toUInt();

    if ( !m_collection.isNull() )
        connect( m_collection->source().data(), SIGNAL( offline() ), SIGNAL( becomingUnavailable() ), Qt::QueuedConnection );
}


float
Result::score() const
{
    return m_v.toMap().value( "score", 0.0 ).toFloat();
}


RID
Result::id() const
{
    if ( m_rid.isEmpty() )
    {
        m_rid = m_v.toMap().value( "sid" ).toString();
    }
    return m_rid;
};
