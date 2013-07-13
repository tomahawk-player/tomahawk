/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "NetworkActivityWorker_p.h"

#include "database/Database.h"
#include "database/DatabaseCommand_TrendingTracks.h"
#include "database/DatabaseImpl.h"
#include "utils/Logger.h"
#include "NetworkActivityWidget.h"

namespace Tomahawk {

NetworkActivityWorker::NetworkActivityWorker( QObject* parent )
    : QThread( parent )
    , d_ptr( new NetworkActivityWorkerPrivate( this ) )
{
}


NetworkActivityWorker::~NetworkActivityWorker()
{
}


void
NetworkActivityWorker::run()
{
    tLog() << Q_FUNC_INFO;
    {
        // Load trending tracks
        qRegisterMetaType< QList< QPair< double,Tomahawk::track_ptr > > >("QList< QPair< double,Tomahawk::track_ptr > >");
        DatabaseCommand_TrendingTracks* dbcmd = new DatabaseCommand_TrendingTracks();
        dbcmd->setLimit( TRENDING_TRACKS_NUM );
        connect( dbcmd, SIGNAL( done( QList< QPair< double,Tomahawk::track_ptr > >) ),
                 SLOT( trendingTracksReceived( QList< QPair< double,Tomahawk::track_ptr > > ) ), Qt::QueuedConnection );
        Database::instance()->enqueue( dbcmd_ptr( dbcmd ) );
    }

    // Start the event loop
    exec();
}


void
NetworkActivityWorker::trendingTracksReceived( const QList<QPair<double, Tomahawk::track_ptr> >& _tracks)
{
    Q_D( NetworkActivityWorker );
    d->trendingTracksDone = true;

    QList<track_ptr> tracks;
    QList< QPair< double, track_ptr > >::const_iterator iter = _tracks.constBegin();
    const QList< QPair< double, track_ptr > >::const_iterator end = _tracks.constEnd();
    for(; iter != end; ++iter)
    {
        tracks << iter->second;
    }
    emit trendingTracks( tracks );

    checkDone();
}

void
NetworkActivityWorker::checkDone()
{
    Q_D( NetworkActivityWorker );
    if ( d->trendingTracksDone )
    {
        quit();
    }
}

} // namespace Tomahawk
