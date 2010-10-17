#ifndef QUERY_H
#define QUERY_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <QVariant>

#include "tomahawk/collection.h"
#include "tomahawk/result.h"
#include "tomahawk/typedefs.h"

namespace Tomahawk
{

class Query : public QObject
{
Q_OBJECT

public:
    explicit Query( const QVariant& v )
        : m_v( v )
        , m_solved( false )
    {
        // ensure a QID is present:
        QVariantMap m = m_v.toMap();
        if( !m.contains("qid") )
            m.insert( "qid", uuid() );

        m_v = m;
    }

    QVariant toVariant() const { return m_v; }

    /// returns list of all results so far
    QList< result_ptr > results() const;

    /// how many results found so far?
    unsigned int numResults() const;

    QID id() const;

    /// sorter for list of results
    static bool resultSorter( const result_ptr &left, const result_ptr& right );

    /// solved=true when a perfect result has been found (score of 1.0)
    bool solved() const { return m_solved; }

    unsigned int lastPipelineWeight() const { return m_lastpipelineweight; }
    void setLastPipelineWeight( unsigned int w ) { m_lastpipelineweight = w;}

    /// for debug output:
    QString toString() const
    {
        return QString( "Query(%1, %2 - %3)" ).arg( id() ).arg( artist() ).arg( track() );
    }

    QString artist() const { return m_v.toMap().value( "artist" ).toString(); }
    QString album()  const { return m_v.toMap().value( "album" ).toString(); }
    QString track()  const { return m_v.toMap().value( "track" ).toString(); }

signals:
    void resultsAdded( const QList<Tomahawk::result_ptr>& );
    void resultsRemoved( Tomahawk::result_ptr );
    void solvedStateChanged( bool state );

public slots:
    /// (indirectly) called by resolver plugins when results are found
    void addResults( const QList< Tomahawk::result_ptr >& );
    void removeResult( Tomahawk::result_ptr );

private slots:
    void resultUnavailable();

private:
    mutable QMutex m_mut;
    QVariant m_v;
    QList< Tomahawk::result_ptr > m_results;
    bool m_solved;
    mutable QID m_qid;
    unsigned int m_lastpipelineweight;
};

}; //ns

#endif // QUERY_H
