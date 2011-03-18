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

#include "databasecommand_loadfile.h"

#include "databaseimpl.h"
#include "collection.h"


DatabaseCommand_LoadFile::DatabaseCommand_LoadFile( const QString& id, QObject* parent )
    : DatabaseCommand( parent )
    , m_id( id )
{
}


void
DatabaseCommand_LoadFile::exec( DatabaseImpl* dbi )
{
    Tomahawk::result_ptr r;
    // file ids internally are really ints, at least for now:
    bool ok;
    do
    {
        unsigned int fid = m_id.toInt( &ok );
        if( !ok )
            break;

        r = dbi->file( fid );
    } while( false );

    emit result( r );
}
