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
    bool becameSolved = false;
    {
//        QMutexLocker lock( &m_mut );
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
    qStableSort( m_results.begin(), m_results.end(), Query::resultSorter );
    checkResults();

    emit resultsChanged();
}


void
Query::removeResult( const Tomahawk::result_ptr& result )
{
    {
//        QMutexLocker lock( &m_mut );
        m_results.removeAll( result );
    }

    emit resultsRemoved( result );
    checkResults();
}


void
Query::onResolvingFinished()
{
//    qDebug() << Q_FUNC_INFO << "Finished resolving." << toString();
    emit resolvingFinished( m_solved );
}


QList< result_ptr >
Query::results() const
{
//    QMutexLocker lock( &m_mut );
    return m_results;
}


unsigned int
Query::numResults() const
{
//    QMutexLocker lock( &m_mut );
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
    return left->score() > right->score();
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

    // hook up signals, and check solved status
    foreach( const result_ptr& rp, m_results )
    {
        if ( !m_solved && rp->score() > 0.99 && rp->collection()->source()->isOnline() )
        {
            m_solved = true;
            becameSolved = true;
        }
        if ( rp->score() > 0.99 && rp->collection()->source()->isOnline() )
        {
            becameUnsolved = false;
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
