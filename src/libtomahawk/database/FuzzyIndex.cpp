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

#include "FuzzyIndex.h"

#include <QDir>
#include <QTime>

#include <CLucene.h>
#include <CLucene/queryParser/MultiFieldQueryParser.h>

#include "DatabaseImpl.h"
#include "Database.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "Source.h"

using namespace lucene::analysis;
using namespace lucene::analysis::standard;
using namespace lucene::document;
using namespace lucene::store;
using namespace lucene::index;
using namespace lucene::queryParser;
using namespace lucene::search;


FuzzyIndex::FuzzyIndex( QObject* parent, bool wipe )
    : QObject( parent )
    , m_luceneReader( 0 )
    , m_luceneSearcher( 0 )
{
    QString m_lucenePath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" );
    QByteArray path = m_lucenePath.toUtf8();
    const char* cPath = path.constData();

    bool failed = false;
    tDebug() << "Opening Lucene directory:" << path;
    try
    {
        m_analyzer = _CLNEW SimpleAnalyzer();
        m_luceneDir = FSDirectory::getDirectory( cPath );
    }
    catch ( CLuceneError& error )
    {
        tDebug() << "Caught CLucene error:" << error.what();
        failed = true;
    }

    if ( failed )
    {
        tDebug() << "Initializing RAM directory instead.";

        m_luceneDir = _CLNEW RAMDirectory();
        wipe = true;
    }

    if ( wipe )
        wipeIndex();
}


FuzzyIndex::~FuzzyIndex()
{
    delete m_luceneSearcher;
    delete m_luceneReader;
    delete m_analyzer;
    delete m_luceneDir;
}


bool
FuzzyIndex::wipeIndex()
{
    tLog( LOGVERBOSE ) << "Wiping fuzzy index...";
    beginIndexing();
    endIndexing();

    QTimer::singleShot( 0, this, SLOT( updateIndex() ) );

    return true; // FIXME
}


void
FuzzyIndex::updateIndex()
{
    DatabaseCommand* cmd = new DatabaseCommand_UpdateSearchIndex();
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
FuzzyIndex::beginIndexing()
{
    m_mutex.lock();

    try
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Starting indexing.";
        if ( m_luceneReader != 0 )
        {
            tDebug( LOGVERBOSE ) << "Deleting old lucene stuff.";

            m_luceneSearcher->close();
            m_luceneReader->close();
            delete m_luceneSearcher;
            delete m_luceneReader;
            m_luceneSearcher = 0;
            m_luceneReader = 0;
        }

        tDebug( LOGVERBOSE ) << "Creating new index writer.";
        m_luceneWriter = new IndexWriter( m_luceneDir, m_analyzer, true );
    }
    catch( CLuceneError& error )
    {
        tDebug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }
}


void
FuzzyIndex::endIndexing()
{
    m_luceneWriter->optimize();
    m_luceneWriter->close();
    delete m_luceneWriter;
    m_luceneWriter = 0;

    m_mutex.unlock();
    emit indexReady();
}


void
FuzzyIndex::appendFields( const IndexData& data )
{
    try
    {
        Document doc;

        if ( !data.track.isEmpty() )
        {
            doc.add( *( _CLNEW Field( _T( "fulltext" ), DatabaseImpl::sortname( QString( "%1 %2" ).arg( data.artist ).arg( data.track ) ).toStdWString().c_str(),
                                      Field::STORE_NO | Field::INDEX_UNTOKENIZED ) ) );

            doc.add( *( _CLNEW Field( _T( "track" ), DatabaseImpl::sortname( data.track ).toStdWString().c_str(),
                                      Field::STORE_NO | Field::INDEX_UNTOKENIZED ) ) );

            doc.add( *( _CLNEW Field( _T( "artist" ), DatabaseImpl::sortname( data.artist ).toStdWString().c_str(),
                                      Field::STORE_NO | Field::INDEX_UNTOKENIZED ) ) );

            doc.add( *( _CLNEW Field( _T( "artistid" ), QString::number( data.artistId ).toStdWString().c_str(),
                                      Field::STORE_YES | Field::INDEX_NO ) ) );

            doc.add( *( _CLNEW Field( _T( "trackid" ), QString::number( data.id ).toStdWString().c_str(),
                                      Field::STORE_YES | Field::INDEX_NO ) ) );
        }
        else if ( !data.album.isEmpty() )
        {
            doc.add( *( _CLNEW Field( _T( "album" ), DatabaseImpl::sortname( data.album ).toStdWString().c_str(),
                                      Field::STORE_NO | Field::INDEX_UNTOKENIZED ) ) );

            doc.add( *( _CLNEW Field( _T( "albumid" ), QString::number( data.id ).toStdWString().c_str(),
                                      Field::STORE_YES | Field::INDEX_NO ) ) );
        }
        else
            Q_ASSERT( false );

        m_luceneWriter->addDocument( &doc );
    }
    catch( CLuceneError& error )
    {
        tDebug() << "Caught CLucene error:" << error.what();

        QTimer::singleShot( 0, this, SLOT( wipeIndex() ) );
    }
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
            if ( !IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "index didn't exist.";
                return resultsmap;
            }

            m_luceneReader = IndexReader::open( m_luceneDir );
            m_luceneSearcher = _CLNEW IndexSearcher( m_luceneReader );
        }

        float minScore;
        const TCHAR** fields = 0;
        MultiFieldQueryParser parser( fields, m_analyzer );
        BooleanQuery* qry = _CLNEW BooleanQuery();

        if ( query->isFullTextQuery() )
        {
            QString escapedQuery = QString::fromWCharArray( parser.escape( DatabaseImpl::sortname( query->fullTextQuery() ).toStdWString().c_str() ) );

            Term* term = _CLNEW Term( _T( "track" ), escapedQuery.toStdWString().c_str() );
            Query* fqry = _CLNEW FuzzyQuery( term );
            qry->add( fqry, true, BooleanClause::SHOULD );

            term = _CLNEW Term( _T( "artist" ), escapedQuery.toStdWString().c_str() );
            fqry = _CLNEW FuzzyQuery( term );
            qry->add( fqry, true, BooleanClause::SHOULD );

            term = _CLNEW Term( _T( "fulltext" ), escapedQuery.toStdWString().c_str() );
            fqry = _CLNEW FuzzyQuery( term );
            qry->add( fqry, true, BooleanClause::SHOULD );

            minScore = 0.00;
        }
        else
        {
            QString track = QString::fromWCharArray( parser.escape( DatabaseImpl::sortname( query->track() ).toStdWString().c_str() ) );
            QString artist = QString::fromWCharArray( parser.escape( DatabaseImpl::sortname( query->artist() ).toStdWString().c_str() ) );
//            QString album = QString::fromWCharArray( parser.escape( query->album().toStdWString().c_str() ) );

            Term* term = _CLNEW Term( _T( "track" ), track.toStdWString().c_str() );
            Query* fqry = _CLNEW FuzzyQuery( term );
            qry->add( fqry, true, BooleanClause::MUST );

            term = _CLNEW Term( _T( "artist" ), artist.toStdWString().c_str() );
            fqry = _CLNEW FuzzyQuery( term );
            qry->add( fqry, true, BooleanClause::MUST );

            minScore = 0.00;
        }

        Hits* hits = m_luceneSearcher->search( qry );
        for ( uint i = 0; i < hits->length(); i++ )
        {
            Document* d = &hits->doc( i );

            float score = hits->score( i );
            int id = QString::fromWCharArray( d->get( _T( "trackid" ) ) ).toInt();

            if ( score > minScore )
            {
                resultsmap.insert( id, score );
//                tDebug() << "Index hit:" << id << score << QString::fromWCharArray( ((Query*)qry)->toString() );
            }
        }

        delete hits;
        delete qry;
    }
    catch( CLuceneError& error )
    {
        tDebug() << "Caught CLucene error:" << error.what() << query->toString();

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
            if ( !IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "index didn't exist.";
                return resultsmap;
            }

            m_luceneReader = IndexReader::open( m_luceneDir );
            m_luceneSearcher = _CLNEW IndexSearcher( m_luceneReader );
        }

        QueryParser parser( _T( "album" ), m_analyzer );
        QString escapedName = QString::fromWCharArray( parser.escape( DatabaseImpl::sortname( query->fullTextQuery() ).toStdWString().c_str() ) );

        Query* qry = _CLNEW FuzzyQuery( _CLNEW Term( _T( "album" ), escapedName.toStdWString().c_str() ) );
        Hits* hits = m_luceneSearcher->search( qry );
        for ( uint i = 0; i < hits->length(); i++ )
        {
            Document* d = &hits->doc( i );

            float score = hits->score( i );
            int id = QString::fromWCharArray( d->get( _T( "albumid" ) ) ).toInt();

            if ( score > 0.30 )
            {
                resultsmap.insert( id, score );
//                tDebug() << "Index hit:" << id << score;
            }
        }

        delete hits;
        delete qry;
    }
    catch( CLuceneError& error )
    {
        tDebug() << "Caught CLucene error:" << error.what();

        QTimer::singleShot( 0, this, SLOT( wipeIndex() ) );
    }

    return resultsmap;
}
