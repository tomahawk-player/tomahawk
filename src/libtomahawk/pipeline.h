/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

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
    virtual ~Pipeline();

    unsigned int pendingQueryCount() const { return m_queries_pending.count(); }
    unsigned int activeQueryCount() const { return m_qidsState.count(); }

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
    void resolve( const query_ptr& q, bool prioritized = false );
    void resolve( const QList<query_ptr>& qlist, bool prioritized = false );
    void resolve( QID qid, bool prioritized = false );

    void start();
    void stop();
    void databaseReady();

signals:
    void idle();
    void resolving( const Tomahawk::query_ptr& query );
    
    void resolverAdded( Resolver* );
    void resolverRemoved( Resolver* );

private slots:
    void timeoutShunt( const query_ptr& q );
    void shunt( const query_ptr& q );
    void shuntNext();

private:
    void setQIDState( const Tomahawk::query_ptr& query, int state );
    int incQIDState( const Tomahawk::query_ptr& query );
    int decQIDState( const Tomahawk::query_ptr& query );

    QList< Resolver* > m_resolvers;

    QMap< QID, bool > m_qidsTimeout;
    QMap< QID, unsigned int > m_qidsState;
    QMap< QID, query_ptr > m_qids;
    QMap< RID, result_ptr > m_rids;

    QMutex m_mut; // for m_qids, m_rids

    // store queries here until DB index is loaded, then shunt them all
    QList< query_ptr > m_queries_pending;
    bool m_running;

    static Pipeline* s_instance;
};

}; //ns

#endif // PIPELINE_H
