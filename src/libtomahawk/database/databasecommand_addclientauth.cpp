/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "databasecommand_addclientauth.h"

DatabaseCommand_AddClientAuth::DatabaseCommand_AddClientAuth( const QString& clientToken, 
                                                              const QString& website, 
                                                              const QString& name, 
                                                              const QString& userAgent,
                                                              QObject* parent )
    : DatabaseCommand( parent )
    , m_clientToken( clientToken )
    , m_website( website )
    , m_name( name )
    , m_userAgent( userAgent )
{
}

void DatabaseCommand_AddClientAuth::exec(DatabaseImpl* lib)
{
    TomahawkSqlQuery q = lib->newquery();
    q.prepare( "INSERT INTO http_client_auth (token, website, name, ua, mtime, permissions) VALUES (?, ?, ?, ?, ?, ?)" );
    q.addBindValue( m_clientToken );
    q.addBindValue( m_website );
    q.addBindValue( m_name );
    q.addBindValue( m_userAgent );
    q.addBindValue( 0 );
    q.addBindValue( "*" );
    
    if( !q.exec() ) {
        qWarning() << "Failed to insert http client into auth table!";
    }
}
