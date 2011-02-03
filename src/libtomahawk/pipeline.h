#ifndef PIPELINE_H
#define PIPELINE_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QMutex>

#include "typedefs.h"
#include "query.h"
#include "result.h"
#include "resolver.h"

#include "dllmacro.h"

namespace Tomahawk
{

class Resolver;

class DLLEXPORT Pipeline : public QObject
{
Q_OBJECT

public:
    static Pipeline* instance();

    explicit Pipeline( QObject* parent = 0 );

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
    void resolve( const query_ptr& q, bool prioritized = true );
    void resolve( const QList<query_ptr>& qlist, bool prioritized = true );
    void resolve( QID qid, bool prioritized = true );
    void databaseReady();

signals:
    void idle();

private slots:
    void shunt( const query_ptr& q );
    void shuntNext();

    void indexReady();

private:
    QList< Resolver* > m_resolvers;

    QMap< QID, unsigned int > m_qidsState;
    QMap< QID, query_ptr > m_qids;
    QMap< RID, result_ptr > m_rids;

    QMutex m_mut; // for m_qids, m_rids

    // store queries here until DB index is loaded, then shunt them all
    QList< query_ptr > m_queries_pending;
    bool m_index_ready;

    static Pipeline* s_instance;
};

}; //ns

#endif // PIPELINE_H
