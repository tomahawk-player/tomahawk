/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#pragma once
#ifndef PIPELINE_P_H
#define PIPELINE_P_H

#include "Pipeline.h"

#include <QMutex>
#include <QTimer>

namespace Tomahawk
{

class PipelinePrivate
{
public:
    PipelinePrivate( Pipeline* q )
        : q_ptr( q )
        , running( false )
    {
    }

    Pipeline* q_ptr;
    Q_DECLARE_PUBLIC( Pipeline )

private:
    QList< Resolver* > resolvers;
    QList< QPointer<Tomahawk::ExternalResolver> > scriptResolvers;
    QList< ResolverFactoryFunc > resolverFactories;
    QMap< QID, bool > qidsTimeout;
    QMap< QID, unsigned int > qidsState;
    QMap< QID, query_ptr > qids;
    QMap< RID, result_ptr > rids;

    QMutex mut; // for m_qids, m_rids

    // store queries here until DB index is loaded, then shunt them all
    QList< query_ptr > queries_pending;
    // store temporary queries here and clean up after timeout threshold
    QList< query_ptr > queries_temporary;

    int maxConcurrentQueries;
    bool running;
    QTimer temporaryQueryTimer;

    static Pipeline* s_instance;
};

} // Tomahawk

#endif // PIPELINE_P_H
