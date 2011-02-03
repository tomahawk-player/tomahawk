#include "query.h"

#include <QtAlgorithms>

#include "database/database.h"
#include "pipeline.h"
#include "sourcelist.h"

using namespace Tomahawk;


query_ptr
Query::get( const QVariant& v, bool autoResolve )
{
    query_ptr q = query_ptr( new Query( v ) );

    if ( autoResolve )
        Pipeline::instance()->resolve( q );
    return q;
}


Query::Query( const QVariant& v )
    : m_v( v )
    , m_solved( false )
{
    QVariantMap m = m_v.toMap();

    m_artist = m.value( "artist" ).toString();
    m_album = m.value( "album" ).toString();
    m_track = m.value( "track" ).toString();

    m_qid = m.value( "qid" ).toString();

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( refreshResults() ), Qt::QueuedConnection );
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
            connect( rp.data(), SIGNAL( becomingUnavailable() ), SLOT( resultUnavailable() ) );
            if( !m_solved && rp->score() > 0.99 )
            {
                m_solved = true;
                becameSolved = true;
            }
        }
    }
    emit resultsAdded( newresults );
    if( becameSolved )
        emit solvedStateChanged( true );
}


void
Query::refreshResults()
{
    Pipeline::instance()->resolve( id() );
}


void
Query::resultUnavailable()
{
    Result* result = (Result*) sender();
    Q_ASSERT( result );

    for ( int i = 0; i < m_results.length(); ++i )
    {
        if ( m_results.value( i ).data() == result )
        {
            result_ptr r  = m_results.value( i );
            m_results.removeAt( i );

            emit resultsRemoved( r );
            break;
        }
    }

    if ( m_results.isEmpty() )  // FIXME proper score checking
        emit solvedStateChanged( false );
}


void
Query::removeResult( const Tomahawk::result_ptr& result )
{
    {
        QMutexLocker lock( &m_mut );
        m_results.removeAll( result );
    }
    emit resultsRemoved( result );

    if ( m_results.isEmpty() )  // FIXME proper score checking
        emit solvedStateChanged( false );
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

