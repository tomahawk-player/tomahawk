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

#include "DatabaseCommand_DirMtimes.h"


#include "DatabaseImpl.h"
#include "utils/Logger.h"
#include "Source.h"
#include "database/TomahawkSqlQuery.h"

void
DatabaseCommand_DirMtimes::exec( DatabaseImpl* dbi )
{
    if( m_update )
        execUpdate( dbi );
    else
        execSelect( dbi );
}


void
DatabaseCommand_DirMtimes::execSelect( DatabaseImpl* dbi )
{
    QMap<QString,unsigned int> mtimes;
    TomahawkSqlQuery query = dbi->newquery();
    if( m_prefix.isEmpty() && m_prefixes.isEmpty() )
    {
        query.exec( "SELECT name, mtime FROM dirs_scanned" );
        while( query.next() )
            mtimes.insert( query.value( 0 ).toString(), query.value( 1 ).toUInt() );
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
DatabaseCommand_DirMtimes::execSelectPath( DatabaseImpl *dbi, const QDir& path, QMap<QString, unsigned int> &mtimes )
{
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( QString( "SELECT name, mtime "
                            "FROM dirs_scanned "
                            "WHERE name LIKE :prefix" ) );

    query.bindValue( ":prefix", path.canonicalPath() + "%" );
    query.exec();

    while( query.next() )
        mtimes.insert( query.value( 0 ).toString(), query.value( 1 ).toUInt() );
}


void
DatabaseCommand_DirMtimes::execUpdate( DatabaseImpl* dbi )
{
    qDebug() << "Saving mtimes...";
    TomahawkSqlQuery query = dbi->newquery();
    query.exec( "DELETE FROM dirs_scanned" );
    query.prepare( "INSERT INTO dirs_scanned(name, mtime) VALUES(?, ?)" );

    foreach( const QString& k, m_tosave.keys() )
    {
        query.bindValue( 0, k );
        query.bindValue( 1, m_tosave.value( k ) );
        query.exec();
    }

    qDebug() << "Saved mtimes for" << m_tosave.size() << "dirs.";
    emit done( QMap< QString, unsigned int >() );
}
