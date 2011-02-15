#include "sourcelist.h"

#include <QDebug>

#include "network/controlconnection.h"

using namespace Tomahawk;

SourceList* SourceList::s_instance = 0;


SourceList*
SourceList::instance()
{
    if ( !s_instance )
    {
        s_instance = new SourceList();
    }
    
    return s_instance;
}

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
        if( s->isLocal() )
        {
            Q_ASSERT( m_local.isNull() );
            m_local = s;
        }

        qDebug() << "SourceList::add(" << s->userName() << "), total sources now:" << m_sources.size();
    }

    emit sourceAdded( s );
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

        if ( src->controlConnection() )
            src->controlConnection()->shutdown( true );
    }

    emit sourceRemoved( src );
}

void
SourceList::removeAllRemote()
{
    foreach( const source_ptr& s, m_sources )
    {
        if( s != m_local )
            remove( s );
    }
}


QList<source_ptr>
SourceList::sources() const
{
    QMutexLocker lock( &m_mut );
    return m_sources.values();
}


source_ptr
SourceList::get( unsigned int id ) const
{
    QMutexLocker lock( &m_mut );
    return m_sources.value( m_sources_id2name.value( id ) );
}


source_ptr
SourceList::get( const QString& username ) const
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
