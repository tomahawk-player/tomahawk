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

#include "DatabaseCommand_SourceOffline.h"

#include "DatabaseImpl.h"
#include "TomahawkSqlQuery.h"
#include "utils/Logger.h"
#include "Source.h"


DatabaseCommand_SourceOffline::DatabaseCommand_SourceOffline( int id )
    : DatabaseCommand()
    , m_id( id )
{
}


void
DatabaseCommand_SourceOffline::exec( DatabaseImpl* lib )
{
    TomahawkSqlQuery q = lib->newquery();
    q.exec( QString( "UPDATE source SET isonline = 'false' WHERE id = %1" )
            .arg( m_id ) );
}
