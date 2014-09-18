/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "FuzzyIndex.h"

#include "utils/Logger.h"

#include "database/DatabaseImpl.h"
#include "PlaylistEntry.h"
#include "Source.h"
#include "Track.h"

#include <QDir>
#include <QTime>
#include <QTimer>

#include <lucene++/FuzzyQuery.h>

using namespace Lucene;


FuzzyIndex::FuzzyIndex( QObject* parent, const QString& filename, bool wipe )
    : QObject( parent )
{
    m_lucenePath = TomahawkUtils::appDataDir().absoluteFilePath( filename );

    bool failed = false;
    tDebug() << "Opening Lucene directory:" << m_lucenePath;
    try
    {
        m_analyzer = newLucene<SimpleAnalyzer>();
        m_luceneDir = FSDirectory::open( StringUtils::toString( m_lucenePath.toUtf8().constData() ) );
//        m_luceneDir = FSDirectory::open( m_lucenePath.toStdWString() );
    }
    catch ( LuceneException& error )
    {
        tDebug() << "Caught Lucene error:" << error.what();
        failed = true;
    }

    if ( failed )
    {
        deleteIndex();
        wipe = true;
    }

    if ( wipe )
        wipeIndex();
}


FuzzyIndex::~FuzzyIndex()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
}


bool
FuzzyIndex::wipeIndex()
{
    tLog( LOGVERBOSE ) << "Wiping fuzzy index:" << m_lucenePath;
    beginIndexing();
    endIndexing();

    QTimer::singleShot( 0, this, SLOT( updateIndexSlot() ) );

    return true; // FIXME
}


void
FuzzyIndex::updateIndexSlot()
{
    updateIndex();
}


void
FuzzyIndex::beginIndexing()
{
    m_mutex.lock();

    try
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Starting indexing:" << m_lucenePath;
        if ( m_luceneReader )
        {
            tDebug( LOGVERBOSE ) << "Deleting old lucene stuff.";

            m_luceneSearcher->close();
            m_luceneReader->close();
            m_luceneSearcher.reset();
            m_luceneReader.reset();
        }

        tDebug( LOGVERBOSE ) << "Creating new index writer.";
        m_luceneWriter = newLucene<IndexWriter>( m_luceneDir, m_analyzer, true, IndexWriter::MaxFieldLengthLIMITED );
    }
    catch( LuceneException& error )
    {
        tDebug() << "Caught Lucene error:" << error.what();
        Q_ASSERT( false );
    }
}


void
FuzzyIndex::endIndexing()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Finishing indexing:" << m_lucenePath;
    m_luceneWriter->optimize();
    m_luceneWriter->close();
    m_luceneWriter.reset();

    m_mutex.unlock();
    emit indexReady();
}


void
FuzzyIndex::appendFields( const Tomahawk::IndexData& data )
{
    try
    {
        DocumentPtr doc = newLucene<Document>();

        if ( !data.track.isEmpty() )
        {
            doc->add(newLucene<Field>( L"fulltext", Tomahawk::DatabaseImpl::sortname( QString( "%1 %2" ).arg( data.artist ).arg( data.track ) ).toStdWString(),
                                       Field::STORE_NO, Field::INDEX_ANALYZED ) );

            doc->add(newLucene<Field>( L"track", Tomahawk::DatabaseImpl::sortname( data.track ).toStdWString(),
                                       Field::STORE_NO, Field::INDEX_ANALYZED ) );

            doc->add(newLucene<Field>( L"artist", Tomahawk::DatabaseImpl::sortname( data.artist ).toStdWString(),
                                       Field::STORE_NO, Field::INDEX_ANALYZED ) );

            doc->add(newLucene<Field>( L"artistid", QString::number( data.artistId ).toStdWString(),
                                       Field::STORE_YES, Field::INDEX_NO ) );

            doc->add(newLucene<Field>( L"trackid", QString::number( data.id ).toStdWString(),
                                       Field::STORE_YES, Field::INDEX_NO ) );
        }
        else if ( !data.album.isEmpty() )
        {
            doc->add(newLucene<Field>( L"album", Tomahawk::DatabaseImpl::sortname( data.album ).toStdWString(),
                                       Field::STORE_NO, Field::INDEX_ANALYZED ) );

            doc->add(newLucene<Field>( L"albumid", QString::number( data.id ).toStdWString(),
                                       Field::STORE_YES, Field::INDEX_NO ) );
        }
        else
            return;

        m_luceneWriter->addDocument( doc );
    }
    catch( LuceneException& error )
    {
        tDebug() << "Caught Lucene error:" << error.what();

        QTimer::singleShot( 0, this, SLOT( wipeIndex() ) );
    }
}


void
FuzzyIndex::deleteIndex()
{
    if ( m_luceneReader )
    {
        tDebug( LOGVERBOSE ) << "Deleting old lucene stuff.";

        m_luceneSearcher->close();
        m_luceneReader->close();
        m_luceneSearcher.reset();
        m_luceneReader.reset();
    }

    TomahawkUtils::removeDirectory( m_lucenePath );
}


void
FuzzyIndex::updateIndex()
{
    // virtual NO-OP
}


void
FuzzyIndex::loadLuceneIndex()
{
    emit indexReady();
}


QMap< int, float >
FuzzyIndex::search( const Tomahawk::query_ptr& query )
{
    QMutexLocker lock( &m_mutex );

    QMap< int, float > resultsmap;
    try
    {
        if ( !m_luceneReader )
        {
            if ( !IndexReader::indexExists( m_luceneDir ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "index didn't exist.";
                return resultsmap;
            }

            m_luceneReader = IndexReader::open( m_luceneDir );
            m_luceneSearcher = newLucene<IndexSearcher>( m_luceneReader );
        }

        float minScore;
        Collection<String> fields; // = newCollection<String>();
        MultiFieldQueryParserPtr parser = newLucene<MultiFieldQueryParser>( LuceneVersion::LUCENE_CURRENT, fields, m_analyzer );
        BooleanQueryPtr qry = newLucene<BooleanQuery>();

        if ( query->isFullTextQuery() )
        {
            QString q = Tomahawk::DatabaseImpl::sortname( query->fullTextQuery() );

            FuzzyQueryPtr fqry = newLucene<FuzzyQuery>( newLucene<Term>( L"track", q.toStdWString() ) );
            qry->add( boost::dynamic_pointer_cast<Query>( fqry ), BooleanClause::SHOULD );

            fqry = newLucene<FuzzyQuery>( newLucene<Term>( L"artist", q.toStdWString() ) );
            qry->add( boost::dynamic_pointer_cast<Query>( fqry ), BooleanClause::SHOULD );

            fqry = newLucene<FuzzyQuery>( newLucene<Term>( L"fulltext", q.toStdWString() ) );
            qry->add( boost::dynamic_pointer_cast<Query>( fqry ), BooleanClause::SHOULD );

            minScore = 0.00;
        }
        else
        {
            QString track = Tomahawk::DatabaseImpl::sortname( query->queryTrack()->track() );
            QString artist = Tomahawk::DatabaseImpl::sortname( query->queryTrack()->artist() );
            //QString album = Tomahawk::DatabaseImpl::sortname( query->queryTrack()->album() );

            FuzzyQueryPtr fqry = newLucene<FuzzyQuery>( newLucene<Term>( L"track", track.toStdWString() ) );
            qry->add( boost::dynamic_pointer_cast<Query>( fqry ), BooleanClause::MUST );

            fqry = newLucene<FuzzyQuery>( newLucene<Term>( L"artist", artist.toStdWString() ) );
            qry->add( boost::dynamic_pointer_cast<Query>( fqry ), BooleanClause::MUST );

            minScore = 0.00;
        }

        TopScoreDocCollectorPtr collector = TopScoreDocCollector::create( 50, false );
        m_luceneSearcher->search( qry, collector );
        Collection<ScoreDocPtr> hits = collector->topDocs()->scoreDocs;

        for ( int i = 0; i < collector->getTotalHits() && i < 50; i++ )
        {
            DocumentPtr d = m_luceneSearcher->doc( hits[i]->doc );
            float score = hits[i]->score;
            int id = QString::fromStdWString( d->get( L"trackid" ) ).toInt();

            if ( score > minScore )
            {
                resultsmap.insert( id, score );
//                tDebug() << "Index hit:" << id << score << QString::fromWCharArray( ((Query*)qry)->toString() );
            }
        }
    }
    catch( LuceneException& error )
    {
        tDebug() << "Caught Lucene error:" << error.what() << query->toString();

        QTimer::singleShot( 0, this, SLOT( wipeIndex() ) );
    }

    return resultsmap;
}


QMap< int, float >
FuzzyIndex::searchAlbum( const Tomahawk::query_ptr& query )
{
    Q_ASSERT( query->isFullTextQuery() );

    QMutexLocker lock( &m_mutex );

    QMap< int, float > resultsmap;
    try
    {
        if ( !m_luceneReader )
        {
            if ( !IndexReader::indexExists( m_luceneDir ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "index didn't exist.";
                return resultsmap;
            }

            m_luceneReader = IndexReader::open( m_luceneDir );
            m_luceneSearcher = newLucene<IndexSearcher>( m_luceneReader );
        }

        QueryParserPtr parser = newLucene<QueryParser>( LuceneVersion::LUCENE_CURRENT, L"album", m_analyzer );
        QString q = Tomahawk::DatabaseImpl::sortname( query->fullTextQuery() );

        FuzzyQueryPtr qry = newLucene<FuzzyQuery>( newLucene<Term>( L"album", q.toStdWString() ) );
        TopScoreDocCollectorPtr collector = TopScoreDocCollector::create( 99999, false );
        m_luceneSearcher->search( boost::dynamic_pointer_cast<Query>( qry ), collector );
        Collection<ScoreDocPtr> hits = collector->topDocs()->scoreDocs;

        for ( int i = 0; i < collector->getTotalHits(); i++ )
        {
            DocumentPtr d = m_luceneSearcher->doc( hits[i]->doc );
            float score = hits[i]->score;
            int id = QString::fromStdWString( d->get( L"albumid" ) ).toInt();

            if ( score > 0.30 )
            {
                resultsmap.insert( id, score );
//                tDebug() << "Index hit:" << id << score;
            }
        }
    }
    catch( LuceneException& error )
    {
        tDebug() << "Caught Lucene error:" << error.what();

        QTimer::singleShot( 0, this, SLOT( wipeIndex() ) );
    }

    return resultsmap;
}
