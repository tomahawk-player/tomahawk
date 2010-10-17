#include "tomahawk/result.h"

using namespace Tomahawk;


Result::Result( QVariant v, collection_ptr collection )
    : m_v( v )
    , m_collection( collection )
{
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
