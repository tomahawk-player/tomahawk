#include "sourcelist.h"

#include <QDebug>

#include "database/database.h"
#include "database/databasecommand_loadallsources.h"
#include "network/remotecollection.h"
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
    loadSources();
}


const source_ptr&
SourceList::getLocal()
{
    return m_local;
}


void
SourceList::loadSources()
{
    qDebug() << Q_FUNC_INFO;
    DatabaseCommand_LoadAllSources* cmd = new DatabaseCommand_LoadAllSources();
    
    connect( cmd, SIGNAL( done( const QList<Tomahawk::source_ptr>& ) ),
                    SLOT( setSources( const QList<Tomahawk::source_ptr>& ) ) );
    
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
SourceList::setSources( const QList<Tomahawk::source_ptr>& sources )
{
    QMutexLocker lock( &m_mut );

    foreach( const source_ptr& src, sources )
    {
        add( src );
    }

    qDebug() << Q_FUNC_INFO << "- Total sources now:" << m_sources.size();
}


void
SourceList::setLocal( const Tomahawk::source_ptr& localSrc )
{
    Q_ASSERT( localSrc->isLocal() );
    Q_ASSERT( m_local.isNull() );
    
    {
        QMutexLocker lock( &m_mut );
        m_sources.insert( localSrc->userName(), localSrc );
        m_local = localSrc;

        qDebug() << Q_FUNC_INFO << localSrc->userName();
    }

    emit sourceAdded( localSrc );
}


void
SourceList::add( const source_ptr& source )
{
    Q_ASSERT( source->id() );

    connect( source.data(), SIGNAL( syncedWithDatabase() ), SLOT( sourceSynced() ) );
    m_sources.insert( source->userName(), source );
    m_sources_id2name.insert( source->id(), source->userName() );
    
    collection_ptr coll( new RemoteCollection( source ) );
    source->addCollection( coll );
    
    emit sourceAdded( source );
}


void
SourceList::removeAllRemote()
{
    foreach( source_ptr s, m_sources )
    {
        if( s != m_local )
        {
            if ( s->controlConnection() )
            {
                s->controlConnection()->shutdown( true );
            }
        }
    }
}


QList<source_ptr>
SourceList::sources( bool onlyOnline ) const
{
    QMutexLocker lock( &m_mut );

    QList< source_ptr > sources;
    foreach( const source_ptr& src, m_sources )
    {
        if ( !onlyOnline || src->controlConnection() )
            sources << src;
    }

    return sources;
}


source_ptr
SourceList::get( unsigned int id ) const
{
    QMutexLocker lock( &m_mut );
    return m_sources.value( m_sources_id2name.value( id ) );
}


source_ptr
SourceList::get( const QString& username, const QString& friendlyName )
{
    QMutexLocker lock( &m_mut );

    source_ptr source;
    if ( !m_sources.contains( username ) )
    {
        source = source_ptr( new Source( -1, username ) );
        source->setFriendlyName( friendlyName );
        add( source );
    }
    else
        source = m_sources.value( username );

    return source;
}


void
SourceList::sourceSynced()
{
    Source* src = qobject_cast< Source* >( sender() );

    Q_ASSERT( m_sources_id2name.values().contains( src->userName() ) );
    m_sources_id2name.remove( m_sources_id2name.key( src->userName() ) );
    m_sources_id2name.insert( src->id(), src->userName() );
}


unsigned int
SourceList::count() const
{
    QMutexLocker lock( &m_mut );
    return m_sources.size();
}
