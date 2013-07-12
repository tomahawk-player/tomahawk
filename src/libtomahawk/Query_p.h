#ifndef QUERY_P_H
#define QUERY_P_H

#include "Query.h"

#include <QMutex>

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
        , qid( _qid )
        , queryTrack( track )
    {
    }

    QueryPrivate( Query* q, const QString& query, const QID& _qid )
        : q_ptr( q )
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
    bool solved;
    bool playable;
    bool resolveFinished;
    mutable QID qid;

    QString fullTextQuery;

    QString resultHint;
    bool saveResultHint;

    QList< QPointer< Tomahawk::Resolver > > resolvers;

    track_ptr queryTrack;

    mutable QMutex mutex;
    QWeakPointer< Tomahawk::Query > ownRef;
};

} // Tomahawk

#endif // QUERY_P_H
