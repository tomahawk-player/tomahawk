#include "query.h"

#include "collection.h"
#include <QtAlgorithms>

#include "database/database.h"
#include "pipeline.h"
#include "sourcelist.h"

using namespace Tomahawk;


query_ptr
Query::get( const QVariant& v, bool autoResolve )
{
    query_ptr q = query_ptr( new Query( v, autoResolve ) );

    if ( autoResolve )
        Pipeline::instance()->resolve( q );
    return q;
}


Query::Query( const QVariant& v, bool autoResolve )
    : m_v( v )
    , m_solved( false )
{
    QVariantMap m = m_v.toMap();

    m_artist = m.value( "artist" ).toString();
    m_album = m.value( "album" ).toString();
    m_track = m.value( "track" ).toString();

    m_qid = m.value( "qid" ).toString();

    if ( autoResolve )
        connect( Database::instance(), SIGNAL( indexReady() ), SLOT( refreshResults() ), Qt::QueuedConnection );
}


void
Query::addResults( const QList< Tomahawk::result_ptr >& newresults )
{
    bool becameSolved = false;
    {
        QMutexLocker lock( &m_mut );
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
        QMutexLocker lock( &m_mut );
        m_results.removeAll( result );
    }

    emit resultsRemoved( result );
    checkResults();
}


void
Query::onResolvingFinished()
{
//    qDebug() << Q_FUNC_INFO << "Finished resolving." << toString();
    emit resolvingFinished( !m_results.isEmpty() );
}


QList< result_ptr >
Query::results() const
{
    QMutexLocker lock( &m_mut );
    return m_results;
}


unsigned int
Query::numResults() const
{
    QMutexLocker lock( &m_mut );
    return m_results.length();
}


QID
Query::id() const
{
    if ( m_qid.isEmpty() )
    {
        m_qid = uuid();

        QVariantMap m = m_v.toMap();
        m.insert( "qid", m_qid );

        m_v = m;
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
        if ( !m_solved && rp->score() > 0.99 )
        {
            m_solved = true;
            becameSolved = true;
        }
        if ( rp->score() > 0.99 )
        {
            becameUnsolved = false;
        }
    }

    if ( m_solved && becameUnsolved )
    {
        m_solved = false;
        emit solvedStateChanged( true );
    }

    if( becameSolved )
        emit solvedStateChanged( true );
}
