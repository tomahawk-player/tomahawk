#ifndef PIPELINE_H
#define PIPELINE_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QMutex>

#include "tomahawk/typedefs.h"
#include "tomahawk/query.h"
#include "tomahawk/result.h"
#include "tomahawk/resolver.h"

namespace Tomahawk
{

class Resolver;

class Pipeline : public QObject
{
Q_OBJECT

public:
    explicit Pipeline( QObject* parent = 0 );

//    const query_ptr& query( QID qid ) const;
//    result_ptr result( RID rid ) const;

    void reportResults( QID qid, const QList< result_ptr >& results );

    /// sorter to rank resolver priority
    static bool resolverSorter( const Resolver* left, const Resolver* right );

    void addResolver( Resolver* r, bool sort = true );
    void removeResolver( Resolver* r );

    query_ptr query( const QID& qid ) const
    {
        return m_qids.value( qid );
    }

    result_ptr result( const RID& rid ) const
    {
        return m_rids.value( rid );
    }

public slots:
    void add( const query_ptr& q );
    void add( const QList<query_ptr>& qlist );
    void databaseReady();

private slots:
    void shunt( const query_ptr& q );
    void indexReady();

private:
    QList< Resolver* > m_resolvers;
    QMap< QID, query_ptr > m_qids;
    QMap< RID, result_ptr > m_rids;

    QMutex m_mut; // for m_qids, m_rids

    // store queries here until DB index is loaded, then shunt them all
    QList< query_ptr > m_queries_pending;
    bool m_index_ready;
};

}; //ns

#endif // PIPELINE_H
