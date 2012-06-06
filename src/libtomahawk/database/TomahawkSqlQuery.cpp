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

#include "database/TomahawkSqlQuery.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#include <QSqlError>
#include <QTime>
#include <QThread>
#include <QVariant>

#define QUERY_THRESHOLD 60


TomahawkSqlQuery::TomahawkSqlQuery()
    : QSqlQuery()
{
}


TomahawkSqlQuery::TomahawkSqlQuery( const QSqlDatabase& db )
    : QSqlQuery( db )
    , m_db( db )
{
}


bool
TomahawkSqlQuery::exec( const QString& query )
{
    prepare( query );
    return exec();
}


bool
TomahawkSqlQuery::exec()
{
    QTime t;
    t.start();

    unsigned int retries = 0;
    while ( !QSqlQuery::exec() && ++retries < 10 )
    {
        tDebug() << "INFO: Retrying failed query:" << this->lastQuery() << this->lastError().text();
        TomahawkUtils::msleep( 25 );
    }

    bool ret = ( retries < 10 );
    if ( !ret )
        showError();

    int e = t.elapsed();
    bool log = ( e >= QUERY_THRESHOLD );

#ifdef TOMAHAWK_QUERY_ANALYZE
    log = true;
#endif
    
    if ( log )
        tLog( LOGSQL ) << "TomahawkSqlQuery (" << t.elapsed() << "ms ):" << lastQuery();
    
    return ret;
}


bool
TomahawkSqlQuery::commitTransaction()
{
    unsigned int retries = 0;
    while ( !m_db.commit() && ++retries < 10 )
    {
        tDebug() << "INFO: Retrying failed commit:" << this->lastQuery() << this->lastError().text();
        TomahawkUtils::msleep( 25 );
    }
    
    return ( retries < 10 );
}


void
TomahawkSqlQuery::showError()
{
    tLog() << "\n" << "*** DATABASE ERROR ***" << "\n"
           << this->lastQuery() << "\n"
           << "boundValues:" << this->boundValues() << "\n"
           << this->lastError().text() << "\n";

    Q_ASSERT( false );
}
