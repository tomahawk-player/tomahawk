#ifndef QUERY_H
#define QUERY_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <QVariant>

#include "result.h"
#include "typedefs.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT Query : public QObject
{
Q_OBJECT

public:
    static query_ptr get( const QVariant& v, bool autoResolve = true );
    explicit Query( const QVariant& v, bool autoResolve );

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
    void setLastPipelineWeight( unsigned int w ) { m_lastpipelineweight = w; }

    /// for debug output:
    QString toString() const
    {
        return QString( "Query(%1, %2 - %3)" ).arg( id() ).arg( artist() ).arg( track() );
    }

    QString artist() const { return m_artist; }
    QString album()  const { return m_album; }
    QString track()  const { return m_track; }

signals:
    void resultsAdded( const QList<Tomahawk::result_ptr>& );
    void resultsRemoved( const Tomahawk::result_ptr& );

    void resultsChanged();
    void solvedStateChanged( bool state );
    void resolvingFinished( bool hasResults );
    
public slots:
    /// (indirectly) called by resolver plugins when results are found
    void addResults( const QList< Tomahawk::result_ptr >& );
    void removeResult( const Tomahawk::result_ptr& );

    void onResolvingFinished();

private slots:
    void onResultStatusChanged();
    void refreshResults();

private:
    void clearResults();
    void checkResults();
    
    mutable QMutex m_mut;
    mutable QVariant m_v;
    QList< Tomahawk::result_ptr > m_results;
    bool m_solved;
    mutable QID m_qid;
    unsigned int m_lastpipelineweight;

    QString m_artist;
    QString m_album;
    QString m_track;
};

}; //ns

#endif // QUERY_H
