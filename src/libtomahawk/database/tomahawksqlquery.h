#ifndef TOMAHAWKSQLQUERY_H
#define TOMAHAWKSQLQUERY_H
// subclass QSqlQuery so that it prints the error msg if a query fails

#include <QSqlQuery>
#include <QSqlError>
#include <QTime>

#define TOMAHAWK_QUERY_THRESHOLD 20

class TomahawkSqlQuery : public QSqlQuery
{

public:

    TomahawkSqlQuery()
        : QSqlQuery()
    {}

    TomahawkSqlQuery( QSqlDatabase db )
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
