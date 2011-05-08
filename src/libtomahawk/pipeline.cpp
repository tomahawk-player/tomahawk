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

#include "pipeline.h"

#include <QDebug>
#include <QMutexLocker>
#include <QTimer>

#include "functimeout.h"
#include "database/database.h"

#define CONCURRENT_QUERIES 4

using namespace Tomahawk;

Pipeline* Pipeline::s_instance = 0;


Pipeline*
Pipeline::instance()
{
    return s_instance;
}


Pipeline::Pipeline( QObject* parent )
    : QObject( parent )
    , m_index_ready( false )
{
    s_instance = this;
}


void
Pipeline::databaseReady()
{
    connect( Database::instance(), SIGNAL( indexReady() ), this, SLOT( indexReady() ), Qt::QueuedConnection );
    Database::instance()->loadIndex();
}


void Pipeline::indexReady()
{
    qDebug() << Q_FUNC_INFO << "shunting this many pending queries:" << m_queries_pending.size();
    m_index_ready = true;

    shuntNext();
}


void
Pipeline::removeResolver( Resolver* r )
{
    m_resolvers.removeAll( r );
}


void
Pipeline::addResolver( Resolver* r, bool sort )
{
    m_resolvers.append( r );
    if( sort )
    {
        qSort( m_resolvers.begin(),
               m_resolvers.end(),
               Pipeline::resolverSorter );
    }
    qDebug() << "Adding resolver" << r->name();

/*    qDebug() << "Current pipeline:";
    foreach( Resolver * r, m_resolvers )
    {
        qDebug() << "* score:" << r->weight()
                 << "pref:" << r->preference()
                 << "name:" << r->name();
    }*/
}


void
Pipeline::resolve( const QList<query_ptr>& qlist, bool prioritized )
{
    {
        QMutexLocker lock( &m_mut );

        int i = 0;
        foreach( const query_ptr& q, qlist )
        {
//            qDebug() << Q_FUNC_INFO << (qlonglong)q.data() << q->toString();
            if ( !m_qids.contains( q->id() ) )
            {
                m_qids.insert( q->id(), q );
            }

            if ( m_queries_pending.contains( q ) )
            {
                qDebug() << "Already queued for resolving:" << q->toString();
                continue;
            }

            if ( prioritized )
            {
                m_queries_pending.insert( i++, q );
            }
            else
            {
                m_queries_pending.append( q );
            }
        }
    }

    shuntNext();
}


void
Pipeline::resolve( const query_ptr& q, bool prioritized )
{
    if ( q.isNull() )
        return;

    QList< query_ptr > qlist;
    qlist << q;
    resolve( qlist, prioritized );
}


void
Pipeline::resolve( QID qid, bool prioritized )
{
    resolve( query( qid ), prioritized );
}


void
Pipeline::reportResults( QID qid, const QList< result_ptr >& results )
{
    {
        QMutexLocker lock( &m_mut );

        if ( !m_qids.contains( qid ) )
        {
            qDebug() << "reportResults called for unknown QID" << qid;
            Q_ASSERT( false );
            return;
        }
    }

    const query_ptr& q = m_qids.value( qid );
    if ( !results.isEmpty() )
    {
        //qDebug() << Q_FUNC_INFO << qid;

        q->addResults( results );
        foreach( const result_ptr& r, q->results() )
        {
            m_rids.insert( r->id(), r );
        }

        if ( q->solved() )
        {
            qDebug() << "FINISHED RESOLVING EARLY" << q->toString();
            q->onResolvingFinished();

            setQIDState( q, 0 );
            if ( m_qidsTimeout.contains( q->id() ) )
                m_qidsTimeout.remove( q->id() );

            qDebug() << "Queries running:" << m_qidsState.count();

            shuntNext();
            return;
        }
    }

    if ( decQIDState( q ) == 0 )
    {
        if ( !q->solved() )
            q->onResolvingFinished();

        if ( m_qidsTimeout.contains( q->id() ) )
            m_qidsTimeout.remove( q->id() );

        qDebug() << "Queries running:" << m_qidsState.count();
        shuntNext();
    }
    else
    {
        new FuncTimeout( 500, boost::bind( &Pipeline::timeoutShunt, this, q ), this );
    }
}


void
Pipeline::shuntNext()
{
    if ( !m_index_ready )
        return;

    query_ptr q;
    {
        QMutexLocker lock( &m_mut );

        if ( m_queries_pending.isEmpty() )
        {
            if ( m_qidsState.isEmpty() )
                emit idle();
            return;
        }

//        qDebug() << Q_FUNC_INFO << m_qidsState.count();
        // Check if we are ready to dispatch more queries
        if ( m_qidsState.count() >= CONCURRENT_QUERIES )
            return;

        /*
            Since resolvers are async, we now dispatch to the highest weighted ones
            and after timeout, dispatch to next highest etc, aborting when solved
        */
        q = m_queries_pending.takeFirst();
        q->setLastPipelineWeight( 101 );
    }

    setQIDState( q, m_resolvers.count() );
    new FuncTimeout( 500, boost::bind( &Pipeline::shunt, this, q ), this );
}


void
Pipeline::timeoutShunt( const query_ptr& q )
{
    // are we still waiting for a timeout?
    if ( m_qidsTimeout.contains( q->id() ) )
    {
        m_qidsTimeout.remove( q->id() );
        shunt( q );
    }
    else
        qDebug() << Q_FUNC_INFO << q->toString() << "Ignoring timeout";
}


void
Pipeline::shunt( const query_ptr& q )
{
    qDebug() << Q_FUNC_INFO << q->solved() << q->toString() << q->id();
    unsigned int lastweight = 0;
    unsigned int lasttimeout = 0;
    int thisResolver = 0;

    if ( !q->resolvingFinished() )
    {
        int i = 0;
        foreach( Resolver* r, m_resolvers )
        {
            i++;
            if ( r->weight() >= q->lastPipelineWeight() )
                continue;

            if ( lastweight == 0 )
            {
                lastweight = r->weight();
                lasttimeout = r->timeout();
                //qDebug() << "Shunting into weight" << lastweight << "q:" << q->toString();
            }
            if ( lastweight == r->weight() )
            {
                // snag the lowest timeout at this weight
                if ( r->timeout() < lasttimeout )
                    lasttimeout = r->timeout();

                qDebug() << "Dispatching to resolver" << r->name() << q->toString();
                thisResolver = i;
                r->resolve( q );
            }
            else
                break;
        }
    }

    if ( lastweight > 0 )
    {
        q->setLastPipelineWeight( lastweight );

        if ( thisResolver < m_resolvers.count() )
        {
            qDebug() << "Shunting in" << lasttimeout << "ms, q:" << q->toString();

            m_qidsTimeout.insert( q->id(), true );
            new FuncTimeout( lasttimeout, boost::bind( &Pipeline::timeoutShunt, this, q ), this );
        }
    }
    else
    {
        // reached end of pipeline
        qDebug() << "Reached end of pipeline for:" << q->toString();
        setQIDState( q, 0 );
        qDebug() << "Queries running:" << m_qidsState.count();
        return;
    }

    shuntNext();
}


bool
Pipeline::resolverSorter( const Resolver* left, const Resolver* right )
{
    if( left->weight() == right->weight() ) // TODO dispatch in parallel
        return left;
    else
        return left->weight() > right->weight();
}


void
Pipeline::setQIDState( const Tomahawk::query_ptr& query, int state )
{
    QMutexLocker lock( &m_mut );

    if ( state > 0 )
    {
        qDebug() << Q_FUNC_INFO << "inserting to qidsstate:" << query->id() << state;
        m_qidsState.insert( query->id(), state );
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "removing" << query->id() << state;
        m_qidsState.remove( query->id() );
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

    qDebug() << Q_FUNC_INFO << "inserting to qidsstate:" << query->id() << state;
    m_qidsState.insert( query->id(), state );

    return state;
}


int
Pipeline::decQIDState( const Tomahawk::query_ptr& query )
{
    QMutexLocker lock( &m_mut );

    if ( !m_qidsState.contains( query->id() ) )
        return 0;

    int state = m_qidsState.value( query->id() ) - 1;
    if ( state )
    {
        qDebug() << Q_FUNC_INFO << "replacing" << query->id() << state;
        m_qidsState.insert( query->id(), state );
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "removing" << query->id() << state;
        m_qidsState.remove( query->id() );
    }

    return state;
}
