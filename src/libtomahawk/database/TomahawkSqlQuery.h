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

#ifndef TOMAHAWKSQLQUERY_H
#define TOMAHAWKSQLQUERY_H

// subclass QSqlQuery so that it prints the error msg if a query fails

#include <QSqlDriver>
#include <QSqlQuery>

//#define TOMAHAWK_QUERY_ANALYZE 1

#include "DllMacro.h"

class DLLEXPORT TomahawkSqlQuery : public QSqlQuery
{

public:
    TomahawkSqlQuery();
    TomahawkSqlQuery( const QSqlDatabase& db );

    static QString escape( QString identifier );

    bool prepare( const QString& query );
    bool exec( const QString& query );
    bool exec();

    bool commitTransaction();

private:
    bool isBusyError( const QSqlError& error ) const;

    void showError();

    QSqlDatabase m_db;
    QString m_query;
};

#endif // TOMAHAWKSQLQUERY_H
