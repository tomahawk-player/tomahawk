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

#include "databasecommand_clientauthvalid.h"

DatabaseCommand_ClientAuthValid::DatabaseCommand_ClientAuthValid( const QString& clientToken, QObject* parent )
    : DatabaseCommand( parent )
    , m_clientToken( clientToken )
{

}

void DatabaseCommand_ClientAuthValid::exec(DatabaseImpl* lib)
{
    TomahawkSqlQuery q = lib->newquery();
    q.prepare(  "SELECT name FROM http_client_auth WHERE token = ?" );
    q.addBindValue( m_clientToken );
    
    if( q.exec() ) {
        if( q.next() ) {
            QString name = q.value( 0 ).toString();
            emit authValid( m_clientToken, name, true );
        } else {
            emit authValid( m_clientToken, QString(), false );
        }
    } else {
        qWarning() << "Failed to query http auth table for client:" << m_clientToken;
    }
}
