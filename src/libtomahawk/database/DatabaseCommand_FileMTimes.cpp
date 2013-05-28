/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DatabaseCommand_FileMTimes.h"

#include "DatabaseImpl.h"
#include "utils/Logger.h"
#include "Source.h"

#include "database/TomahawkSqlQuery.h"

void
DatabaseCommand_FileMtimes::exec( DatabaseImpl* dbi )
{
    execSelect( dbi );
}


void
DatabaseCommand_FileMtimes::execSelect( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    //FIXME: If ever needed for a non-local source this will have to be fixed/updated
    QMap< QString, QMap< unsigned int, unsigned int > > mtimes;
    TomahawkSqlQuery query = dbi->newquery();
    if( m_prefix.isEmpty() && m_prefixes.isEmpty() )
    {
        QString limit( m_checkonly ? QString( "LIMIT 1" ) : QString() );
        query.exec( QString( "SELECT url, id, mtime FROM file WHERE source IS NULL %1" ).arg( limit ) );
        while( query.next() )
        {
            QMap< unsigned int, unsigned int > map;
            map.insert( query.value( 1 ).toUInt(), query.value( 2 ).toUInt() );
            mtimes.insert( query.value( 0 ).toString(), map );
        }
    }
    else if( m_prefixes.isEmpty() )
        execSelectPath( dbi, m_prefix, mtimes );
    else
    {
        if( !m_prefix.isEmpty() )
            execSelectPath( dbi, m_prefix, mtimes );
        foreach( QString path, m_prefixes )
            execSelectPath( dbi, path, mtimes );
    }
    emit done( mtimes );
}

void
DatabaseCommand_FileMtimes::execSelectPath( DatabaseImpl *dbi, const QDir& path, QMap<QString, QMap< unsigned int, unsigned int > > &mtimes )
{
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( QString( "SELECT url, id, mtime "
                            "FROM file "
                            "WHERE source IS NULL "
                            "AND url LIKE :prefix" ) );

    query.bindValue( ":prefix", "file://" + path.canonicalPath() + "%" );
    query.exec();

    while( query.next() )
    {
        QMap< unsigned int, unsigned int > map;
        map.insert( query.value( 1 ).toUInt(), query.value( 2 ).toUInt() );
        mtimes.insert( query.value( 0 ).toString(), map );
    }
}
