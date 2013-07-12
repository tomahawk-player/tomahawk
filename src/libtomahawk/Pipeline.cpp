/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "Pipeline_p.h"

#include <QMutexLocker>

#include "database/Database.h"
#include "resolvers/ExternalResolver.h"
#include "resolvers/ScriptResolver.h"
#include "resolvers/JSResolver.h"
#include "utils/ResultUrlChecker.h"
#include "utils/Logger.h"

#include "FuncTimeout.h"
#include "Result.h"
#include "Source.h"
#include "SourceList.h"

#include <boost/bind.hpp>

#define DEFAULT_CONCURRENT_QUERIES 4
#define MAX_CONCURRENT_QUERIES 16
#define CLEANUP_TIMEOUT 5 * 60 * 1000
#define MINSCORE 0.5

using namespace Tomahawk;

Pipeline* PipelinePrivate::s_instance = 0;


Pipeline*
Pipeline::instance()
{
    return PipelinePrivate::s_instance;
}


Pipeline::Pipeline( QObject* parent )
    : QObject( parent )
    , d_ptr( new PipelinePrivate( this ) )
{
    Q_D( Pipeline );
    PipelinePrivate::s_instance = this;

    d->maxConcurrentQueries = qBound( DEFAULT_CONCURRENT_QUERIES, QThread::idealThreadCount(), MAX_CONCURRENT_QUERIES );
    tDebug() << Q_FUNC_INFO << "Using" << d->maxConcurrentQueries << "threads";

    d->temporaryQueryTimer.setInterval( CLEANUP_TIMEOUT );
    connect( &d->temporaryQueryTimer, SIGNAL( timeout() ), SLOT( onTemporaryQueryTimer() ) );

    connect( this, SIGNAL( resolverAdded( Tomahawk::Resolver* ) ),
             SourceList::instance(), SLOT( onResolverAdded( Tomahawk::Resolver* ) ) );
    connect( this, SIGNAL( resolverRemoved( Tomahawk::Resolver* ) ),
             SourceList::instance(), SLOT( onResolverRemoved( Tomahawk::Resolver* ) ) );
}


Pipeline::~Pipeline()
{
    Q_D( Pipeline );
    tDebug() << Q_FUNC_INFO;
    d->running = false;

    // stop script resolvers
    foreach ( QPointer< ExternalResolver > r, d->scriptResolvers )
        if ( !r.isNull() )
            r.data()->deleteLater();

    d->scriptResolvers.clear();
}


bool
Pipeline::isRunning() const
{
    Q_D( const Pipeline );
    return d->running;
}


unsigned int
Pipeline::pendingQueryCount() const
{
    Q_D( const Pipeline );
    return d->queries_pending.count();
}


unsigned int
Pipeline::activeQueryCount() const
{
    Q_D( const Pipeline );
    return d->qidsState.count();
}


void
Pipeline::databaseReady()
{
    connect( Database::instance(), SIGNAL( ready() ), this, SLOT( start() ), Qt::QueuedConnection );
    Database::instance()->loadIndex();
}


void
Pipeline::start()
{
    Q_D( Pipeline );

    tDebug() << Q_FUNC_INFO << "Shunting" << d->queries_pending.size() << "queries!";
    d->running = true;
    emit running();

    shuntNext();
}


void
Pipeline::stop()
{
    Q_D( Pipeline );

    d->running = false;
}


void
Pipeline::removeResolver( Resolver* r )
{
    Q_D( Pipeline );
    QMutexLocker lock( &d->mut );

    tDebug() << "Removed resolver:" << r->name();
    d->resolvers.removeAll( r );
    emit resolverRemoved( r );
}


void
Pipeline::addResolver( Resolver* r )
{
    Q_D( Pipeline );
    QMutexLocker lock( &d->mut );

    tDebug() << "Adding resolver" << r->name();
    d->resolvers.append( r );
    emit resolverAdded( r );
}


void
Pipeline::addExternalResolverFactory( ResolverFactoryFunc resolverFactory )
{
    Q_D( Pipeline );
    d->resolverFactories << resolverFactory;
}


Tomahawk::ExternalResolver*
Pipeline::addScriptResolver( const QString& path, const QStringList& additionalScriptPaths )
{
    Q_D( Pipeline );
    ExternalResolver* res = 0;

    foreach ( ResolverFactoryFunc factory, d->resolverFactories )
    {
        res = factory( path, additionalScriptPaths );
        if ( !res )
            continue;

        d->scriptResolvers << QPointer< ExternalResolver > ( res );

        break;
    }

    return res;
}


void
Pipeline::stopScriptResolver( const QString& path )
{
    Q_D( Pipeline );
    foreach ( QPointer< ExternalResolver > res, d->scriptResolvers )
    {
        if ( res.data()->filePath() == path )
            res.data()->stop();
    }
}


void
Pipeline::removeScriptResolver( const QString& scriptPath )
{
    Q_D( Pipeline );
    QPointer< ExternalResolver > r;
    foreach ( QPointer< ExternalResolver > res, d->scriptResolvers )
    {
        if ( res.isNull() )
            continue;

        if ( res.data()->filePath() == scriptPath )
            r = res;
    }
    d->scriptResolvers.removeAll( r );

    if ( !r.isNull() )
    {
        r.data()->stop();
        r.data()->deleteLater();
    }
}


QList<QPointer<ExternalResolver> >
Pipeline::scriptResolvers() const
{
    Q_D( const Pipeline );

    return d->scriptResolvers;
}


ExternalResolver*
Pipeline::resolverForPath( const QString& scriptPath )
{
    Q_D( Pipeline );

    foreach ( QPointer< ExternalResolver > res, d->scriptResolvers )
    {
        if ( res.data()->filePath() == scriptPath )
            return res.data();
    }
    return 0;
}


void
Pipeline::resolve( const QList<query_ptr>& qlist, bool prioritized, bool temporaryQuery )
{
    Q_D( Pipeline );

    {
        QMutexLocker lock( &d->mut );

        int i = 0;
        foreach ( const query_ptr& q, qlist )
        {
            if ( q->resolvingFinished() )
                continue;
            if ( d->qidsState.contains( q->id() ) )
                continue;
            if ( d->queries_pending.contains( q ) )
            {
                if ( prioritized )
                {
                    d->queries_pending.insert( i++, d->queries_pending.takeAt( d->queries_pending.indexOf( q ) ) );
                }
                continue;
            }

            if ( !d->qids.contains( q->id() ) )
                d->qids.insert( q->id(), q );

            if ( prioritized )
                d->queries_pending.insert( i++, q );
            else
                d->queries_pending << q;

            if ( temporaryQuery )
            {
                d->queries_temporary << q;

                if ( d->temporaryQueryTimer.isActive() )
                    d->temporaryQueryTimer.stop();
                d->temporaryQueryTimer.start();
            }
        }
    }

    shuntNext();
}


bool
Pipeline::isResolving( const query_ptr& q ) const
{
    Q_D( const Pipeline );

    return d->qids.contains( q->id() ) && d->qidsState.contains( q->id() );
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
    Q_D( Pipeline );
    if ( !d->running )
        return;
    if ( !d->qids.contains( qid ) )
    {
        tDebug() << "Result arrived too late for:" << qid;
        return;
    }
    const query_ptr& q = d->qids.value( qid );

    Q_ASSERT( !q.isNull() );
    if ( q.isNull() )
        return;

    QList< result_ptr > cleanResults;
    QList< result_ptr > httpResults;
    foreach ( const result_ptr& r, results )
    {
        if ( r.isNull() )
            continue;

        if ( !r->checked() && ( r->url().startsWith( "http" ) && !r->url().startsWith( "http://localhost" ) ) )
            httpResults << r;
        else
            cleanResults << r;
    }const

    ResultUrlChecker* checker = new ResultUrlChecker( q, httpResults );
    connect( checker, SIGNAL( done() ), SLOT( onResultUrlCheckerDone() ) );

    addResultsToQuery( q, cleanResults );
    if ( q->solved() && !q->isFullTextQuery() )
    {
        setQIDState( q, 0 );
        return;
    }

    if ( httpResults.isEmpty() )
        decQIDState( q );
}


void
Pipeline::addResultsToQuery( const query_ptr& query, const QList< result_ptr >& results )
{
    Q_D( Pipeline );
//    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << query->toString() << results.count();

    QList< result_ptr > cleanResults;
    foreach ( const result_ptr& r, results )
    {
        r->setScore( query->howSimilar( r ) );
        if ( !query->isFullTextQuery() && r->score() < MINSCORE )
            continue;

        cleanResults << r;
    }

    if ( !cleanResults.isEmpty() )
    {
        query->addResults( cleanResults );

        if ( d->queries_temporary.contains( query ) )
        {
            foreach ( const result_ptr& r, cleanResults )
            {
                d->rids.insert( r->id(), r );
            }
        }
    }
}


void
Pipeline::onResultUrlCheckerDone()
{
    ResultUrlChecker* checker = qobject_cast< ResultUrlChecker* >( sender() );
    if ( !checker )
        return;

    checker->deleteLater();

    const query_ptr q = checker->query();
    addResultsToQuery( q, checker->validResults() );
    if ( q && !q->isFullTextQuery() )
    {
        setQIDState( q, 0 );
        return;
    }

    decQIDState( q );
}


void
Pipeline::reportAlbums( QID qid, const QList< album_ptr >& albums )
{
    Q_D( Pipeline );
    if ( !d->running )
        return;

    if ( !d->qids.contains( qid ) )
    {
        tDebug() << "Albums arrived too late for:" << qid;
        return;
    }
    const query_ptr& q = d->qids.value( qid );
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
    Q_D( Pipeline );
    if ( !d->running )
        return;

    if ( !d->qids.contains( qid ) )
    {
        tDebug() << "Artists arrived too late for:" << qid;
        return;
    }
    const query_ptr& q = d->qids.value( qid );
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
    Q_D( Pipeline );
    if ( !d->running )
        return;

    unsigned int rc;
    query_ptr q;
    {
        QMutexLocker lock( &d->mut );

        rc = d->resolvers.count();
        if ( d->queries_pending.isEmpty() )
        {
            if ( d->qidsState.isEmpty() )
                emit idle();
            return;
        }

        // Check if we are ready to dispatch more queries
        if ( d->qidsState.count() >= d->maxConcurrentQueries )
            return;

        /*
            Since resolvers are async, we now dispatch to the highest weighted ones
            and after timeout, dispatch to next highest etc, aborting when solved
        */
        q = d->queries_pending.takeFirst();
        q->setCurrentResolver( 0 );
    }

    setQIDState( q, rc );
}


void
Pipeline::timeoutShunt( const query_ptr& q )
{
    Q_D( Pipeline );
    if ( !d->running )
        return;

    // are we still waiting for a timeout?
    if ( d->qidsTimeout.contains( q->id() ) )
    {
        decQIDState( q );
    }
}


void
Pipeline::shunt( const query_ptr& q )
{
    Q_D( Pipeline );
    if ( !d->running )
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
            d->qidsTimeout.insert( q->id(), true );
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
    Q_D( const Pipeline );
    Resolver* newResolver = 0;

    foreach ( Resolver* r, d->resolvers )
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
    Q_D( Pipeline );
    QMutexLocker lock( &d->mut );

    if ( d->qidsTimeout.contains( query->id() ) )
        d->qidsTimeout.remove( query->id() );

    if ( state > 0 )
    {
        d->qidsState.insert( query->id(), state );

        new FuncTimeout( 0, boost::bind( &Pipeline::shunt, this, query ), this );
    }
    else
    {
        d->qidsState.remove( query->id() );
        query->onResolvingFinished();

        if ( !d->queries_temporary.contains( query ) )
            d->qids.remove( query->id() );

        new FuncTimeout( 0, boost::bind( &Pipeline::shuntNext, this ), this );
    }
}


int
Pipeline::incQIDState( const Tomahawk::query_ptr& query )
{
    Q_D( Pipeline );
    QMutexLocker lock( &d->mut );

    int state = 1;
    if ( d->qidsState.contains( query->id() ) )
    {
        state = d->qidsState.value( query->id() ) + 1;
    }
    d->qidsState.insert( query->id(), state );

    return state;
}


int
Pipeline::decQIDState( const Tomahawk::query_ptr& query )
{
    Q_D( Pipeline );
    int state = 0;
    {
        QMutexLocker lock( &d->mut );

        if ( !d->qidsState.contains( query->id() ) )
            return 0;

        state = d->qidsState.value( query->id() ) - 1;
    }

    setQIDState( query, state );
    return state;
}


void
Pipeline::onTemporaryQueryTimer()
{
    Q_D( Pipeline );
    tDebug() << Q_FUNC_INFO;

    QMutexLocker lock( &d->mut );
    d->temporaryQueryTimer.stop();

    for ( int i = d->queries_temporary.count() - 1; i >= 0; i-- )
    {
        query_ptr q = d->queries_temporary.takeAt( i );

        d->qids.remove( q->id() );
        foreach ( const Tomahawk::result_ptr& r, q->results() )
            d->rids.remove( r->id() );
    }
}


query_ptr
Pipeline::query( const QID& qid ) const
{
    Q_D( const Pipeline );
    return d->qids.value( qid );
}


result_ptr
Pipeline::result( const RID& rid ) const
{
    Q_D( const Pipeline );
    return d->rids.value( rid );
}
