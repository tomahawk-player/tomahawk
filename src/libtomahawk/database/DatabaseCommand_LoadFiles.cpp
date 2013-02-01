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

#include "DatabaseCommand_LoadFiles.h"

#include "DatabaseImpl.h"
#include "collection/Collection.h"
#include "utils/Logger.h"
#include "Source.h"


DatabaseCommand_LoadFiles::DatabaseCommand_LoadFiles( unsigned int id, QObject* parent )
    : DatabaseCommand( parent )
    , m_single( true )
{
    m_ids << id;
}

DatabaseCommand_LoadFiles::DatabaseCommand_LoadFiles( const QList<unsigned int>& ids, QObject* parent )
    : DatabaseCommand( parent )
    , m_single( false )
    , m_ids( ids )
{
}


void
DatabaseCommand_LoadFiles::exec( DatabaseImpl* dbi )
{
    QList<Tomahawk::result_ptr> resultList;
    // file ids internally are really ints, at least for now:
    foreach ( unsigned int id, m_ids )
    {
        qDebug() << "Loading file from db with id:" << id;
        resultList << dbi->file( id );
    }

    Q_ASSERT( !m_single || resultList.size() <= 1 );

    if ( m_single && !resultList.isEmpty() )
        emit result( resultList.first() );
    else
        emit results( resultList );
}
