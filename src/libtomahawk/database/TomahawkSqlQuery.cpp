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

#include "database/Database.h"
#include "database/DatabaseImpl.h"
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


QString
TomahawkSqlQuery::escape( QString identifier )
{
    return identifier.replace( "'", "''" );
}


bool
TomahawkSqlQuery::prepare( const QString& query )
{
    m_query = query;
    return QSqlQuery::prepare( query );
}


bool
TomahawkSqlQuery::exec( const QString& query )
{
    bool prepareResult = prepare( query );
    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Query preparation successful?" << ( prepareResult ? "true" : "false" );
    return exec();
}


bool
TomahawkSqlQuery::exec()
{
    bool log = false;
#ifdef TOMAHAWK_QUERY_ANALYZE
    log = true;
#endif
    if ( log )
        tLog( LOGSQL ) << "TomahawkSqlQuery::exec running in thread " << QThread::currentThread();

    QTime t;
    t.start();

    unsigned int retries = 0;
    while ( !QSqlQuery::exec() && ++retries < 10 )
    {
        if ( lastError().text().toLower().contains( "no query" ) ||
             lastError().text().toLower().contains( "parameter count mismatch" ) )
        {
            tDebug() << Q_FUNC_INFO << "Re-preparing query!";

            QMap< QString, QVariant > bv = boundValues();
            prepare( m_query );

            foreach ( const QString& key, bv.keys() )
            {
                tDebug() << Q_FUNC_INFO << "Rebinding key" << key << "with value" << bv.value( key );
                bindValue( key, bv.value( key ) );
            }
        }

        if ( isBusyError( lastError() ) )
            retries = 0;

        tDebug() << "INFO: Retrying failed query:" << lastQuery() << lastError().text();
        TomahawkUtils::msleep( 10 );
    }

    bool ret = ( retries < 10 );
    if ( !ret )
        showError();

    int e = t.elapsed();
    if ( log || e >= QUERY_THRESHOLD )
        tLog( LOGSQL ) << "TomahawkSqlQuery::exec (" << t.elapsed() << "ms ):" << lastQuery();

    return ret;
}


bool
TomahawkSqlQuery::commitTransaction()
{
    bool log = false;
#ifdef TOMAHAWK_QUERY_ANALYZE
    log = true;
#endif
    if ( log )
        tLog( LOGSQL ) << "TomahawkSqlQuery::commitTransaction running in thread " << QThread::currentThread();

    unsigned int retries = 0;
    while ( !m_db.commit() && ++retries < 10 )
    {
        if ( isBusyError( lastError() ) )
            retries = 0;

        tDebug() << "INFO: Retrying failed commit:" << retries << lastError().text();
        TomahawkUtils::msleep( 10 );
    }

    return ( retries < 10 );
}


void
TomahawkSqlQuery::showError()
{
    tLog() << endl << "*** DATABASE ERROR ***" << endl
           << lastQuery() << endl
           << "boundValues:" << boundValues() << endl
           << lastError().text() << endl;

    Q_ASSERT( false );
}


bool
TomahawkSqlQuery::isBusyError( const QSqlError& error ) const
{
    const QString text = error.text().trimmed().toLower();

    return ( text.contains( "locked" ) || text.contains( "busy" ) || text.isEmpty() );
}
