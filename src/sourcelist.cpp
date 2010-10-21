#include "tomahawk/sourcelist.h"

#include <QDebug>

using namespace Tomahawk;

SourceList::SourceList( QObject* parent )
    : QObject( parent )
{
}


const source_ptr&
SourceList::getLocal()
{
    return m_local;
}


void
SourceList::add( const Tomahawk::source_ptr& s )
{
    {
        QMutexLocker lock( &m_mut );
        if ( m_sources.contains( s->userName() ) )
            return;

        m_sources.insert( s->userName(), s );
        if( !s->isLocal() )
        {
            Q_ASSERT( s->id() );
            m_sources_id2name.insert( s->id(), s->userName() );
        }
        qDebug() << "SourceList::add(" << s->userName() << "), total sources now:" << m_sources.size();
        if( s->isLocal() )
        {
            Q_ASSERT( m_local.isNull() );
            m_local = s;
        }
    }
    emit sourceAdded( s );
    s->collection()->loadPlaylists();
}


void
SourceList::remove( const Tomahawk::source_ptr& s )
{
    remove( s.data() );
}


void
SourceList::remove( Tomahawk::Source* s )
{
    qDebug() << Q_FUNC_INFO;
    source_ptr src;
    {
        QMutexLocker lock( &m_mut );
        if ( !m_sources.contains( s->userName() ) )
            return;

        src = m_sources.value( s->userName() );
        m_sources_id2name.remove( src->id() );
        m_sources.remove( s->userName() );
        qDebug() << "SourceList::remove(" << s->userName() << "), total sources now:" << m_sources.size();
    }
    emit sourceRemoved( src );
}


QList<source_ptr>
SourceList::sources() const
{
    QMutexLocker lock( &m_mut );
    return m_sources.values();
}


source_ptr
SourceList::lookup( unsigned int id ) const
{
    QMutexLocker lock( &m_mut );
    return m_sources.value( m_sources_id2name.value( id ) );
}


source_ptr
SourceList::lookup( const QString& username ) const
{
    QMutexLocker lock( &m_mut );
    return m_sources.value( username );
}


unsigned int
SourceList::count() const
{
    QMutexLocker lock( &m_mut );
    return m_sources.size();
}
