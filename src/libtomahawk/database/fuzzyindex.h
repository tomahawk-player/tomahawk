#ifndef FUZZYINDEX_H
#define FUZZYINDEX_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QString>
#include <QMutex>

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

class DatabaseImpl;

class FuzzyIndex : public QObject
{
Q_OBJECT

public:
    explicit FuzzyIndex( DatabaseImpl& db, bool wipeIndex = false );
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
    QString m_lucenePath;

    lucene::analysis::SimpleAnalyzer* m_analyzer;
    lucene::store::Directory* m_luceneDir;
    lucene::index::IndexReader* m_luceneReader;
    lucene::search::IndexSearcher* m_luceneSearcher;
};

#endif // FUZZYINDEX_H
