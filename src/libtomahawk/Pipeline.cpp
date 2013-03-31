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

#include "Pipeline.h"

#include <QMutexLocker>

#include "FuncTimeout.h"
#include "database/Database.h"
#include "resolvers/ExternalResolver.h"
#include "resolvers/ScriptResolver.h"
#include "resolvers/QtScriptResolver.h"
#include "Source.h"
#include "SourceList.h"

#include "utils/Logger.h"

#include "boost/bind.hpp"

#define DEFAULT_CONCURRENT_QUERIES 4
#define MAX_CONCURRENT_QUERIES 16
#define CLEANUP_TIMEOUT 5 * 60 * 1000
#define MINSCORE 0.5

using namespace Tomahawk;

Pipeline* Pipeline::s_instance = 0;


Pipeline*
Pipeline::instance()
{
    return s_instance;
}


Pipeline::Pipeline( QObject* parent )
    : QObject( parent )
    , m_running( false )
{
    s_instance = this;

    m_maxConcurrentQueries = qBound( DEFAULT_CONCURRENT_QUERIES, QThread::idealThreadCount(), MAX_CONCURRENT_QUERIES );
    tDebug() << Q_FUNC_INFO << "Using" << m_maxConcurrentQueries << "threads";

    m_temporaryQueryTimer.setInterval( CLEANUP_TIMEOUT );
    connect( &m_temporaryQueryTimer, SIGNAL( timeout() ), SLOT( onTemporaryQueryTimer() ) );

    connect( this, SIGNAL( resolverAdded( Tomahawk::Resolver* ) ),
             SourceList::instance(), SLOT( onResolverAdded( Tomahawk::Resolver* ) ) );
    connect( this, SIGNAL( resolverRemoved( Tomahawk::Resolver* ) ),
             SourceList::instance(), SLOT( onResolverRemoved( Tomahawk::Resolver* ) ) );
}


Pipeline::~Pipeline()
{
    tDebug() << Q_FUNC_INFO;
    m_running = false;

    // stop script resolvers
    foreach ( QPointer< ExternalResolver > r, m_scriptResolvers )
        if ( !r.isNull() )
            r.data()->deleteLater();

    m_scriptResolvers.clear();
}


void
Pipeline::databaseReady()
{
    connect( Database::instance(), SIGNAL( indexReady() ), this, SLOT( start() ), Qt::QueuedConnection );
    Database::instance()->loadIndex();
}


void
Pipeline::start()
{
    tDebug() << Q_FUNC_INFO << "Shunting this many pending queries:" << m_queries_pending.size();
    m_running = true;

    shuntNext();
}


void
Pipeline::stop()
{
    m_running = false;
}


void
Pipeline::removeResolver( Resolver* r )
{
    QMutexLocker lock( &m_mut );

    tDebug() << "Removed resolver:" << r->name();
    m_resolvers.removeAll( r );
    emit resolverRemoved( r );
}


void
Pipeline::addResolver( Resolver* r )
{
    QMutexLocker lock( &m_mut );

    tDebug() << "Adding resolver" << r->name();
    m_resolvers.append( r );
    emit resolverAdded( r );
}


void
Pipeline::addExternalResolverFactory( ResolverFactoryFunc resolverFactory )
{
    m_resolverFactories << resolverFactory;
}


Tomahawk::ExternalResolver*
Pipeline::addScriptResolver( const QString& path, const QStringList& additionalScriptPaths )
{
    ExternalResolver* res = 0;

    foreach ( ResolverFactoryFunc factory, m_resolverFactories )
    {
        res = factory( path, additionalScriptPaths );
        if ( !res )
            continue;

        m_scriptResolvers << QPointer< ExternalResolver > ( res );

        break;
    }

    return res;
}


void
Pipeline::stopScriptResolver( const QString& path )
{
    foreach ( QPointer< ExternalResolver > res, m_scriptResolvers )
    {
        if ( res.data()->filePath() == path )
            res.data()->stop();
    }
}


void
Pipeline::removeScriptResolver( const QString& scriptPath )
{
    QPointer< ExternalResolver > r;
    foreach ( QPointer< ExternalResolver > res, m_scriptResolvers )
    {
        if ( res.isNull() )
            continue;

        if ( res.data()->filePath() == scriptPath )
            r = res;
    }
    m_scriptResolvers.removeAll( r );

    if ( !r.isNull() )
    {
        r.data()->stop();
        r.data()->deleteLater();
    }
}


ExternalResolver*
Pipeline::resolverForPath( const QString& scriptPath )
{
    foreach ( QPointer< ExternalResolver > res, m_scriptResolvers )
    {
        if ( res.data()->filePath() == scriptPath )
            return res.data();
    }
    return 0;
}


void
Pipeline::resolve( const QList<query_ptr>& qlist, bool prioritized, bool temporaryQuery )
{
    {
        QMutexLocker lock( &m_mut );

        int i = 0;
        foreach ( const query_ptr& q, qlist )
        {
            if ( q->resolvingFinished() )
                continue;
            if ( m_qidsState.contains( q->id() ) )
                continue;
            if ( m_queries_pending.contains( q ) )
            {
                if ( prioritized )
                {
                    m_queries_pending.insert( i++, m_queries_pending.takeAt( m_queries_pending.indexOf( q ) ) );
                }
                continue;
            }

            if ( !m_qids.contains( q->id() ) )
                m_qids.insert( q->id(), q );

            if ( prioritized )
                m_queries_pending.insert( i++, q );
            else
                m_queries_pending << q;

            if ( temporaryQuery )
            {
                m_queries_temporary << q;

                if ( m_temporaryQueryTimer.isActive() )
                    m_temporaryQueryTimer.stop();
                m_temporaryQueryTimer.start();
            }
        }
    }

    shuntNext();
}


bool
Pipeline::isResolving( const query_ptr& q ) const
{
    return m_qids.contains( q->id() ) && m_qidsState.contains( q->id() );
}


void
Pipeline::resolve( const query_ptr& q, bool prioritized, bool temporaryQuery )
{
    if ( q.isNull() )
        return;

    QList< query_ptr > qlist;
    qlist << q;
    resolve( qlist, prioritized, temporaryQuery );
}


void
Pipeline::resolve( QID qid, bool prioritized, bool temporaryQuery )
{
    resolve( query( qid ), prioritized, temporaryQuery );
}


void
Pipeline::reportResults( QID qid, const QList< result_ptr >& results )
{
    if ( !m_running )
        return;

    if ( !m_qids.contains( qid ) )
    {
        tDebug() << "Result arrived too late for:" << qid;
        return;
    }
    const query_ptr& q = m_qids.value( qid );

    Q_ASSERT( !q.isNull() );
    if ( q.isNull() )
        return;

    QList< result_ptr > cleanResults;
    foreach ( const result_ptr& r, results )
    {
        if ( r.isNull() )
            continue;

        float score = q->howSimilar( r );
        r->setScore( score );
        if ( !q->isFullTextQuery() && score < MINSCORE )
            continue;

        cleanResults << r;
    }

    if ( !cleanResults.isEmpty() )
    {
        q->addResults( cleanResults );
        
        if ( m_queries_temporary.contains( q ) )
        {
            foreach ( const result_ptr& r, cleanResults )
            {
                m_rids.insert( r->id(), r );
            }
        }

        if ( q->solved() && !q->isFullTextQuery() )
        {
            setQIDState( q, 0 );
            return;
        }
    }

    decQIDState( q );
}


void
Pipeline::reportAlbums( QID qid, const QList< album_ptr >& albums )
{
    if ( !m_running )
        return;

    if ( !m_qids.contains( qid ) )
    {
        tDebug() << "Albums arrived too late for:" << qid;
        return;
    }
    const query_ptr& q = m_qids.value( qid );
    Q_ASSERT( q->isFullTextQuery() );

    QList< album_ptr > cleanAlbums;
    foreach ( const album_ptr& r, albums )
    {
//        float score = q->howSimilar( r );

        cleanAlbums << r;
    }

    if ( !cleanAlbums.isEmpty() )
    {
        q->addAlbums( cleanAlbums );
    }
}


void
Pipeline::reportArtists( QID qid, const QList< artist_ptr >& artists )
{
    if ( !m_running )
        return;

    if ( !m_qids.contains( qid ) )
    {
        tDebug() << "Artists arrived too late for:" << qid;
        return;
    }
    const query_ptr& q = m_qids.value( qid );
    Q_ASSERT( q->isFullTextQuery() );

    QList< artist_ptr > cleanArtists;
    foreach ( const artist_ptr& r, artists )
    {
//        float score = q->howSimilar( r );

        cleanArtists << r;
    }

    if ( !cleanArtists.isEmpty() )
    {
        q->addArtists( cleanArtists );
    }
}


void
Pipeline::shuntNext()
{
    if ( !m_running )
        return;

    unsigned int rc;
    query_ptr q;
    {
        QMutexLocker lock( &m_mut );

        rc = m_resolvers.count();
        if ( m_queries_pending.isEmpty() )
        {
            if ( m_qidsState.isEmpty() )
                emit idle();
            return;
        }

        // Check if we are ready to dispatch more queries
        if ( m_qidsState.count() >= m_maxConcurrentQueries )
            return;

        /*
            Since resolvers are async, we now dispatch to the highest weighted ones
            and after timeout, dispatch to next highest etc, aborting when solved
        */
        q = m_queries_pending.takeFirst();
        q->setCurrentResolver( 0 );
    }

    setQIDState( q, rc );
}


void
Pipeline::timeoutShunt( const query_ptr& q )
{
    if ( !m_running )
        return;

    // are we still waiting for a timeout?
    if ( m_qidsTimeout.contains( q->id() ) )
    {
        decQIDState( q );
    }
}


void
Pipeline::shunt( const query_ptr& q )
{
    if ( !m_running )
        return;

    Resolver* r = 0;
    if ( !q->resolvingFinished() )
        r = nextResolver( q );

    if ( r )
    {
        tLog( LOGVERBOSE ) << "Dispatching to resolver" << r->name() << q->toString() << q->solved() << q->id();

        q->setCurrentResolver( r );
        r->resolve( q );
        emit resolving( q );

        if ( r->timeout() > 0 )
        {
            m_qidsTimeout.insert( q->id(), true );
            new FuncTimeout( r->timeout(), boost::bind( &Pipeline::timeoutShunt, this, q ), this );
        }
    }
    else
    {
        // we get here if we disable a resolver while a query is resolving
        setQIDState( q, 0 );
        return;
    }

    shuntNext();
}


Tomahawk::Resolver*
Pipeline::nextResolver( const Tomahawk::query_ptr& query ) const
{
    Resolver* newResolver = 0;

    foreach ( Resolver* r, m_resolvers )
    {
        if ( query->resolvedBy().contains( r ) )
            continue;

        if ( !newResolver )
        {
            newResolver = r;
            continue;
        }

        if ( r->weight() > newResolver->weight() )
            newResolver = r;
    }

    return newResolver;
}


void
Pipeline::setQIDState( const Tomahawk::query_ptr& query, int state )
{
    QMutexLocker lock( &m_mut );

    if ( m_qidsTimeout.contains( query->id() ) )
        m_qidsTimeout.remove( query->id() );

    if ( state > 0 )
    {
        m_qidsState.insert( query->id(), state );

        new FuncTimeout( 0, boost::bind( &Pipeline::shunt, this, query ), this );
    }
    else
    {
        m_qidsState.remove( query->id() );
        query->onResolvingFinished();

        if ( !m_queries_temporary.contains( query ) )
            m_qids.remove( query->id() );

        new FuncTimeout( 0, boost::bind( &Pipeline::shuntNext, this ), this );
    }
}


int
Pipeline::incQIDState( const Tomahawk::query_ptr& query )
{
    QMutexLocker lock( &m_mut );

    int state = 1;
    if ( m_qidsState.contains( query->id() ) )
    {
        state = m_qidsState.value( query->id() ) + 1;
    }
    m_qidsState.insert( query->id(), state );

    return state;
}


int
Pipeline::decQIDState( const Tomahawk::query_ptr& query )
{
    int state = 0;
    {
        QMutexLocker lock( &m_mut );

        if ( !m_qidsState.contains( query->id() ) )
            return 0;

        state = m_qidsState.value( query->id() ) - 1;
    }

    setQIDState( query, state );
    return state;
}


void
Pipeline::onTemporaryQueryTimer()
{
    tDebug() << Q_FUNC_INFO;

    QMutexLocker lock( &m_mut );
    m_temporaryQueryTimer.stop();

    for ( int i = m_queries_temporary.count() - 1; i >= 0; i-- )
    {
        query_ptr q = m_queries_temporary.takeAt( i );
        
        m_qids.remove( q->id() );
        foreach ( const Tomahawk::result_ptr& r, q->results() )
            m_rids.remove( r->id() );
    }
}


query_ptr
Pipeline::query( const QID& qid ) const
{
    return m_qids.value( qid );
}


result_ptr
Pipeline::result( const RID& rid ) const
{
    return m_rids.value( rid );
}
