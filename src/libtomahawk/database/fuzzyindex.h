#ifndef FUZZYINDEX_H
#define FUZZYINDEX_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QString>
#include <QMutex>

#ifndef WIN32
namespace lucene
{
    namespace analysis
    {
      class SimpleAnalyzer;
    }
    namespace store
    {
      class Directory;
    }
    namespace index
    {
      class IndexReader;
      class IndexWriter;
    }
    namespace search
    {
      class IndexSearcher;
    }
}
#else
class SimpleAnalyzer;
class Directory;
class IndexReader;
class IndexWriter;
class IndexSearcher;
#endif

class DatabaseImpl;

class FuzzyIndex : public QObject
{
Q_OBJECT

public:
    explicit FuzzyIndex( DatabaseImpl& db );
    ~FuzzyIndex();

    void beginIndexing();
    void endIndexing();
    void appendFields( const QString& table, const QMap< unsigned int, QString >& fields );
    
signals:
    void indexReady();

public slots:
    void loadLuceneIndex();

    QMap< int, float > search( const QString& table, const QString& name );

private:
    DatabaseImpl& m_db;
    QMutex m_mutex;

    #ifndef WIN32
    lucene::analysis::SimpleAnalyzer* m_analyzer;
    lucene::store::Directory* m_luceneDir;
    lucene::index::IndexReader* m_luceneReader;
    lucene::search::IndexSearcher* m_luceneSearcher;
    #else
    SimpleAnalyzer* m_analyzer;
    Directory* m_luceneDir;
    IndexReader* m_luceneReader;
    IndexSearcher* m_luceneSearcher;
    #endif
};

#endif // FUZZYINDEX_H
