#include "fuzzyindex.h"

#include "databaseimpl.h"
#include "utils/tomahawkutils.h"

#include <QDir>
#include <QTime>

#include <CLucene.h>

using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::store;
using namespace lucene::index;
using namespace lucene::queryParser;
using namespace lucene::search;


FuzzyIndex::FuzzyIndex( DatabaseImpl& db, bool wipeIndex )
    : QObject()
    , m_db( db )
    , m_luceneReader( 0 )
    , m_luceneSearcher( 0 )
{
    QString m_lucenePath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" );
    m_luceneDir = FSDirectory::getDirectory( m_lucenePath.toStdString().c_str() );
    m_analyzer = _CLNEW SimpleAnalyzer();

    if ( wipeIndex )
    {
        beginIndexing();
        endIndexing();
    }
}


FuzzyIndex::~FuzzyIndex()
{
    delete m_luceneSearcher;
    delete m_luceneReader;
    delete m_analyzer;
    delete m_luceneDir;
}


void
FuzzyIndex::beginIndexing()
{
    m_mutex.lock();

    try
    {
        qDebug() << Q_FUNC_INFO << "Starting indexing.";
        if ( m_luceneReader != 0 )
        {
            qDebug() << "Deleting old lucene stuff.";
            m_luceneSearcher->close();
            m_luceneReader->close();
            delete m_luceneSearcher;
            delete m_luceneReader;
            m_luceneSearcher = 0;
            m_luceneReader = 0;
        }

        qDebug() << "Creating new index writer.";
        IndexWriter luceneWriter = IndexWriter( m_luceneDir, m_analyzer, true );
    }
    catch( CLuceneError& error )
    {
        qDebug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }
}


void
FuzzyIndex::endIndexing()
{
    m_mutex.unlock();
    emit indexReady();
}


void
FuzzyIndex::appendFields( const QString& table, const QMap< unsigned int, QString >& fields )
{
    try
    {
        qDebug() << "Appending to index:" << fields.count();
        bool create = !IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() );
        IndexWriter luceneWriter = IndexWriter( m_luceneDir, m_analyzer, create );
        Document doc;

        QMapIterator< unsigned int, QString > it( fields );
        while ( it.hasNext() )
        {
            it.next();
            unsigned int id = it.key();
            QString name = it.value();

            {
                Field* field = _CLNEW Field( table.toStdWString().c_str(), name.toStdWString().c_str(),
                                            Field::STORE_YES | Field::INDEX_UNTOKENIZED );
                doc.add( *field );
            }

            {
                Field* field = _CLNEW Field( _T( "id" ), QString::number( id ).toStdWString().c_str(),
                Field::STORE_YES | Field::INDEX_NO );
                doc.add( *field );
            }

            luceneWriter.addDocument( &doc );
            doc.clear();
        }

        luceneWriter.close();
    }
    catch( CLuceneError& error )
    {
        qDebug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }
}


void
FuzzyIndex::loadLuceneIndex()
{
    emit indexReady();
}


QMap< int, float >
FuzzyIndex::search( const QString& table, const QString& name )
{
    QMutexLocker lock( &m_mutex );

    QMap< int, float > resultsmap;
    try
    {
        if ( !m_luceneReader )
        {
            if ( !IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() ) )
            {
                qDebug() << Q_FUNC_INFO << "index didn't exist.";
                return resultsmap;
            }

            m_luceneReader = IndexReader::open( m_luceneDir );
            m_luceneSearcher = _CLNEW IndexSearcher( m_luceneReader );
        }

        if ( name.isEmpty() )
            return resultsmap;

        SimpleAnalyzer analyzer;
        QueryParser parser( table.toStdWString().c_str(), m_analyzer );
        Hits* hits = 0;

        FuzzyQuery* qry = _CLNEW FuzzyQuery( _CLNEW Term( table.toStdWString().c_str(), name.toStdWString().c_str() ) );
        hits = m_luceneSearcher->search( qry );

        for ( int i = 0; i < hits->length(); i++ )
        {
            Document* d = &hits->doc( i );

            float score = hits->score( i );
            int id = QString::fromWCharArray( d->get( _T( "id" ) ) ).toInt();
            QString result = QString::fromWCharArray( d->get( table.toStdWString().c_str() ) );

            if ( result.toLower() == name.toLower() )
                score = 1.0;
            else
                score = qMin( score, (float)0.99 );

            if ( score > 0.05 )
            {
                resultsmap.insert( id, score );
//                qDebug() << "Hitres:" << result << id << score << table << name;
            }
        }

        delete hits;
        delete qry;
    }
    catch( CLuceneError& error )
    {
        qDebug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }

    return resultsmap;
}
