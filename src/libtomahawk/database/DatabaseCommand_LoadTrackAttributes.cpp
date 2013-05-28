/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DatabaseCommand_LoadTrackAttributes.h"

#include "database/Database.h"
#include "DatabaseImpl.h"
#include "network/Servent.h"
#include "Result.h"
#include "utils/Logger.h"
#include "database/TomahawkSqlQuery.h"
#include "SourceList.h"
#include "TrackData.h"

using namespace Tomahawk;


void
DatabaseCommand_LoadTrackAttributes::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( m_track );
    if ( m_track->trackId() == 0 )
        return;

    TomahawkSqlQuery query = dbi->newquery();

    query.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
    query.bindValue( 0, m_track->trackId() );
    query.exec();

    QVariantMap attr;
    while ( query.next() )
    {
        attr[ query.value( 0 ).toString() ] = query.value( 1 ).toString();
    }

    m_track->setAttributes( attr );

    emit done();
}
DatabaseCommand_LoadTrackAttributes::DatabaseCommand_LoadTrackAttributes(const trackdata_ptr& track, QObject* parent)
    : DatabaseCommand( parent ), m_track( track )
{
    setSource( SourceList::instance()->getLocal() );
}

