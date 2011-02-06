#include "fuzzyindex.h"

#include "databaseimpl.h"
#include "utils/tomahawkutils.h"

#include <QDir>
#include <QTime>

#include <CLucene.h>


FuzzyIndex::FuzzyIndex( DatabaseImpl& db )
    : QObject()
    , m_db( db )
    , m_luceneReader( 0 )
    , m_luceneSearcher( 0 )
{
    QString lucenePath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" );
    bool create = !lucene::index::IndexReader::indexExists( lucenePath.toStdString().c_str() );
    m_luceneDir = lucene::store::FSDirectory::getDirectory( lucenePath.toStdString().c_str(), create );

    m_analyzer = _CLNEW lucene::analysis::SimpleAnalyzer();
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
    lucene::index::IndexWriter luceneWriter = lucene::index::IndexWriter( m_luceneDir, m_analyzer, true );
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
    delete m_luceneSearcher;
    delete m_luceneReader;
    m_luceneSearcher = 0;
    m_luceneReader = 0;
    
    bool create = !lucene::index::IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() );
    lucene::index::IndexWriter luceneWriter = lucene::index::IndexWriter( m_luceneDir, m_analyzer, create );
    lucene::document::Document doc;
    
    QMapIterator< unsigned int, QString > it( fields );
    while ( it.hasNext() )
    {
        it.next();
        unsigned int id = it.key();
        QString name = it.value();

        {
            lucene::document::Field* field = _CLNEW lucene::document::Field( table.toStdWString().c_str(), name.toStdWString().c_str(),
                                                                          lucene::document::Field::STORE_YES | lucene::document::Field::INDEX_UNTOKENIZED );
            doc.add( *field );
        }
        
        {
            lucene::document::Field* field = _CLNEW lucene::document::Field( _T( "id" ), QString::number( id ).toStdWString().c_str(),
            lucene::document::Field::STORE_YES | lucene::document::Field::INDEX_NO );
            doc.add( *field );
        }
        
        luceneWriter.addDocument( &doc );
        doc.clear();
    }
    
    luceneWriter.close();
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
    if ( !m_luceneReader )
    {
        if ( !lucene::index::IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() ) )
        {
            qDebug() << Q_FUNC_INFO << "index didn't exist.";
            return resultsmap;
        }

        m_luceneReader = lucene::index::IndexReader::open( m_luceneDir );
        m_luceneSearcher = _CLNEW lucene::search::IndexSearcher( m_luceneReader );
    }

    if ( name.isEmpty() )
        return resultsmap;

    lucene::analysis::SimpleAnalyzer analyzer;
    lucene::queryParser::QueryParser parser( table.toStdWString().c_str(), m_analyzer );
    lucene::search::Hits* hits = 0;

    lucene::search::FuzzyQuery* qry = _CLNEW lucene::search::FuzzyQuery( _CLNEW lucene::index::Term( table.toStdWString().c_str(), name.toStdWString().c_str() ) );
    hits = m_luceneSearcher->search( qry );

    for ( unsigned int i = 0; i < hits->length(); i++ )
    {
        lucene::document::Document* d = &hits->doc( i );

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
//            qDebug() << "Hitres:" << result << id << score << table << name;
        }
    }

    return resultsmap;
}
