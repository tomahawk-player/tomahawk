/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#define DEFAULT_CONCURRENT_QUERIES 4
#define MAX_CONCURRENT_QUERIES 16
#define CLEANUP_TIMEOUT 5 * 60 * 1000
#define MINSCORE 0.5
#define DEFAULT_RESOLVER_TIMEOUT 5000 // 5 seconds

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

    d->maxConcurrentQueries = 24;
    tDebug() << Q_FUNC_INFO << "Using" << d->maxConcurrentQueries << "threads";

    d->temporaryQueryTimer.setInterval( CLEANUP_TIMEOUT );
    connect( &d->temporaryQueryTimer, SIGNAL( timeout() ), SLOT( onTemporaryQueryTimer() ) );
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
    return d->qidsState.uniqueKeys().count();
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
    if ( d->running ) {
        // Only notify if Pipeline is still active.
        emit resolverRemoved( r );
    }
}


QList< Tomahawk::Resolver* >
Pipeline::resolvers() const
{
    Q_D( const Pipeline );

    return d->resolvers;
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
Pipeline::addScriptResolver( const QString& accountId, const QString& path, const QStringList& additionalScriptPaths )
{
    Q_D( Pipeline );
    ExternalResolver* res = 0;

    foreach ( ResolverFactoryFunc factory, d->resolverFactories )
    {
        res = factory( accountId, path, additionalScriptPaths );
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
Pipeline::reportError( QID qid, Tomahawk::Resolver* r )
{
    reportResults( qid, r, QList< result_ptr>() );
}


void
Pipeline::reportResults( QID qid, Tomahawk::Resolver* r, const QList< result_ptr >& results )
{
    Q_D( Pipeline );
    if ( !d->running )
        return;
    if ( !d->qids.contains( qid ) )
    {
        if ( !results.isEmpty() )
        {
            Resolver* resolvedBy = results[0]->resolvedBy();
            if ( resolvedBy )
            {
                tDebug() << "Result arrived too late for:" << qid << "by" << resolvedBy->name();
            }
            else
            {
                tDebug() << "Result arrived too late for:" << qid;
            }
        }
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
        if ( !r )
            continue;

        if ( !r->checked() && ( r->url().startsWith( "http" ) && !r->url().startsWith( "http://localhost" ) ) )
            httpResults << r;
        else
            cleanResults << r;
    }

    addResultsToQuery( q, cleanResults );
    if ( !httpResults.isEmpty() )
    {
        const ResultUrlChecker* checker = new ResultUrlChecker( q, r, httpResults );
        connect( checker, SIGNAL( done() ), SLOT( onResultUrlCheckerDone() ) );
    }
    else
    {
        decQIDState( q, r );
    }

/*    if ( q->solved() && !q->isFullTextQuery() )
    {
        checkQIDState( q, 0 );
        return;
    }*/
}


void
Pipeline::addResultsToQuery( const query_ptr& query, const QList< result_ptr >& results )
{
    Q_D( Pipeline );
//    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << query->toString() << results.count();

    QList< result_ptr > cleanResults;
    foreach ( const result_ptr& r, results )
    {
        if ( !query->isFullTextQuery() && query->howSimilar( r ) < MINSCORE )
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
/*    if ( q && !q->isFullTextQuery() )
    {
        checkQIDState( q, 0 );
        return;
    }*/

    decQIDState( q, reinterpret_cast<Tomahawk::Resolver*>( checker->userData() ) );
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
        if ( activeQueryCount() >= d->maxConcurrentQueries )
            return;

        /*
            Since resolvers are async, we now dispatch to the highest weighted ones
            and after timeout, dispatch to next highest etc, aborting when solved
        */
        q = d->queries_pending.takeFirst();
        q->setCurrentResolver( 0 );
    }

    // Zero-patient, a stub so that query is not resolved until we go through
    // all resolvers
    // As query considered as 'finished trying to resolve' when there are no
    // more qid entries in qidsState we'll put one as sort of 'keep this until
    // we kick off all our resolvers' entry
    // once we kick off all resolvers we'll remove this entry
    incQIDState( q, nullptr );
    checkQIDState( q );
}


void
Pipeline::timeoutShunt( const query_ptr& q, Tomahawk::Resolver* r )
{
    Q_D( Pipeline );
    if ( !d->running )
        return;

    decQIDState( q, r );
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
        tLog( LOGVERBOSE ) << "Dispatching to resolver" << r->name() << r->timeout() << q->toString() << q->solved() << q->id();

        incQIDState( q, r );
        q->setCurrentResolver( r );
        r->resolve( q );
        emit resolving( q );

        auto timeout = r->timeout();
        if ( timeout == 0 )
            timeout = DEFAULT_RESOLVER_TIMEOUT;

        new FuncTimeout( timeout, std::bind( &Pipeline::timeoutShunt, this, q, r ), this );
    }
    else
    {
        // we get here if we disable a resolver while a query is resolving
        // OR we are just out of resolvers while query is still resolving

        // since we seem to at least tried to kick off all of the resolvers,
        // remove the '.keep' entry
        decQIDState( q, nullptr );
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
Pipeline::checkQIDState( const Tomahawk::query_ptr& query )
{
    Q_D( Pipeline );
    QMutexLocker lock( &d->mut );

    tDebug() << Q_FUNC_INFO << query->id() << d->qidsState.count( query->id() );

    if ( d->qidsState.contains( query->id() ) )
    {
        new FuncTimeout( 0, std::bind( &Pipeline::shunt, this, query ), this );
    }
    else
    {
        query->onResolvingFinished();

        if ( !d->queries_temporary.contains( query ) )
            d->qids.remove( query->id() );

        new FuncTimeout( 0, std::bind( &Pipeline::shuntNext, this ), this );
    }
}


void
Pipeline::incQIDState( const Tomahawk::query_ptr& query, Tomahawk::Resolver* r )
{
    Q_D( Pipeline );
    QMutexLocker lock( &d->mut );

    d->qidsState.insert( query->id(), r );
}


void
Pipeline::decQIDState( const Tomahawk::query_ptr& query, Tomahawk::Resolver* r )
{
    Q_D( Pipeline );

    if ( d->qidsState.contains( query->id(), r ) )
    {
        {
            QMutexLocker lock( &d->mut );
            d->qidsState.remove( query->id(), r ); // Removes all matching pairs
        }

        checkQIDState( query );
    }
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
