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

#include <QSqlQuery>
#include <QSqlError>
#include <QTime>

#define TOMAHAWK_QUERY_THRESHOLD 60

class TomahawkSqlQuery : public QSqlQuery
{

public:

    TomahawkSqlQuery()
        : QSqlQuery()
    {}

    TomahawkSqlQuery( const QSqlDatabase& db )
        : QSqlQuery( db )
    {}

    bool exec( const QString& query )
    {
        prepare( query );

        return exec();
    }

    bool exec()
    {
        QTime t;
        t.start();

        bool ret = QSqlQuery::exec();
        if( !ret )
            showError();

        int e = t.elapsed();
        if ( e >= TOMAHAWK_QUERY_THRESHOLD )
            qDebug() << "TomahawkSqlQuery (" << lastQuery() << ") finished in" << t.elapsed() << "ms";

        return ret;
    }

private:
    void showError()
    {
        qDebug()
                << endl << "*** DATABASE ERROR ***" << endl
                << this->lastQuery() << endl
                << "boundValues:" << this->boundValues() << endl
                << this->lastError().text() << endl
                ;
        Q_ASSERT( false );
    }
};

#endif // TOMAHAWKSQLQUERY_H
