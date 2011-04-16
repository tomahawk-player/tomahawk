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

#include "query.h"

#include "collection.h"
#include <QtAlgorithms>

#include "database/database.h"
#include "database/databasecommand_logplayback.h"
#include "database/databasecommand_playbackhistory.h"
#include "database/databasecommand_loadplaylistentries.h"
#include "pipeline.h"
#include "sourcelist.h"

using namespace Tomahawk;


query_ptr
Query::get( const QString& artist, const QString& track, const QString& album, const QID& qid )
{
    query_ptr q = query_ptr( new Query( artist, track, album, qid ) );

    if ( !qid.isEmpty() )
        Pipeline::instance()->resolve( q );
    return q;
}


Query::Query( const QString& artist, const QString& track, const QString& album, const QID& qid )
    : m_solved( false )
    , m_playable( false )
    , m_resolveFinished( false )
    , m_qid( qid )
    , m_artist( artist )
    , m_album( album )
    , m_track( track )
    , m_duration( -1 )
{
    if ( !qid.isEmpty() )
    {
        connect( Database::instance(), SIGNAL( indexReady() ), SLOT( refreshResults() ), Qt::QueuedConnection );
    }
}


void
Query::addResults( const QList< Tomahawk::result_ptr >& newresults )
{
    {
        QMutexLocker lock( &m_mutex );
        m_results.append( newresults );
        qStableSort( m_results.begin(), m_results.end(), Query::resultSorter );

        // hook up signals, and check solved status
        foreach( const result_ptr& rp, newresults )
        {
            connect( rp.data(), SIGNAL( statusChanged() ), SLOT( onResultStatusChanged() ) );
        }
    }

    emit resultsAdded( newresults );
    checkResults();
}


void
Query::refreshResults()
{
    Pipeline::instance()->resolve( id() );
}


void
Query::onResultStatusChanged()
{
    if ( m_results.count() )
        qStableSort( m_results.begin(), m_results.end(), Query::resultSorter );
    checkResults();

    emit resultsChanged();
}


void
Query::removeResult( const Tomahawk::result_ptr& result )
{
    {
        QMutexLocker lock( &m_mutex );
        m_results.removeAll( result );
    }

    emit resultsRemoved( result );
    checkResults();
}


void
Query::onResolvingFinished()
{
//    qDebug() << Q_FUNC_INFO << "Finished resolving." << toString();
    m_resolveFinished = true;
    emit resolvingFinished( m_solved );
}


QList< result_ptr >
Query::results() const
{
    QMutexLocker lock( &m_mutex );
    return m_results;
}


unsigned int
Query::numResults() const
{
    QMutexLocker lock( &m_mutex );
    return m_results.length();
}


QID
Query::id() const
{
    if ( m_qid.isEmpty() )
    {
        m_qid = uuid();
    }

    return m_qid;
}


bool
Query::resultSorter( const result_ptr& left, const result_ptr& right )
{
    const float ls = left->score();
    const float rs = right->score();

    if ( ls == rs )
    {
        if ( !left->collection().isNull() && left->collection()->source()->isLocal() )
            return true;
        else
            return false;
    }

    return ls > rs;
}


void
Query::clearResults()
{
    foreach( const result_ptr& rp, m_results )
    {
        removeResult( rp );
    }
}


void
Query::checkResults()
{
    bool becameSolved = false;
    bool becameUnsolved = true;
    m_playable = false;

    // hook up signals, and check solved status
    foreach( const result_ptr& rp, m_results )
    {
        if ( rp->score() > 0.0 && rp->collection().isNull() )
        {
            m_playable = true;
        }
        if ( !rp->collection().isNull() && rp->collection()->source()->isOnline() )
        {
            m_playable = true;

            if ( rp->score() > 0.99 )
            {
                becameUnsolved = false;

                if ( !m_solved )
                {
                    m_solved = true;
                    becameSolved = true;
                }
            }
        }
    }

    if ( m_solved && becameUnsolved )
    {
        m_solved = false;
        emit solvedStateChanged( false );
    }

    if( becameSolved )
        emit solvedStateChanged( true );
}


QVariant
Query::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", artist() );
    m.insert( "album", album() );
    m.insert( "track", track() );
    m.insert( "duration", duration() );
    m.insert( "qid", id() );
    
    return m;
}


QString
Query::toString() const
{
    return QString( "Query(%1, %2 - %3)" ).arg( id() ).arg( artist() ).arg( track() );
}
