#ifndef QUERY_P_H
#define QUERY_P_H

#include "Query.h"

#include <QMutex>
#include <map>

namespace Tomahawk
{

class QueryPrivate
{
public:
    QueryPrivate( Query* q )
        : q_ptr( q )
    {
    }

    QueryPrivate( Query* q, const track_ptr& track, const QID& _qid )
        : q_ptr( q )
        , allowReresolve( true )
        , qid( _qid )
        , queryTrack( track )
    {
    }

    QueryPrivate( Query* q, const QString& query, const QID& _qid )
        : q_ptr( q )
        , allowReresolve( true )
        , qid( _qid )
        , fullTextQuery( query )
    {
    }

    Q_DECLARE_PUBLIC( Query )
    Query* q_ptr;

private:
    QList< Tomahawk::artist_ptr > artists;
    QList< Tomahawk::album_ptr > albums;
    QList< Tomahawk::result_ptr > results;
    Tomahawk::result_ptr preferredResult;

    float score;
    bool solved;
    bool playable;
    bool resolveFinished;
    bool allowReresolve;
    mutable QID qid;

    QString fullTextQuery;

    QString resultHint;
    bool saveResultHint;

    QList< QPointer< Tomahawk::Resolver > > resolvers;

    track_ptr queryTrack;

    mutable QMutex mutex;
    QWeakPointer< Tomahawk::Query > ownRef;

    std::map<QString, float> howSimilarCache;
};

} // Tomahawk

#endif // QUERY_P_H
