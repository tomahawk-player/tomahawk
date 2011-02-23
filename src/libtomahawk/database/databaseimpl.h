#ifndef DATABASEIMPL_H
#define DATABASEIMPL_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QVariant>
#include <QVariantMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QHash>
#include <QThread>

#include "tomahawksqlquery.h"
#include "fuzzyindex.h"

class Database;

class DatabaseImpl : public QObject
{
Q_OBJECT

friend class FuzzyIndex;
friend class DatabaseCommand_UpdateSearchIndex;

public:
    DatabaseImpl( const QString& dbname, Database* parent = 0 );
    ~DatabaseImpl();

    TomahawkSqlQuery newquery() { return TomahawkSqlQuery( db ); }
    QSqlDatabase& database() { return db; }

    int artistId( const QString& name_orig, bool& isnew );
    int trackId( int artistid, const QString& name_orig, bool& isnew );
    int albumId( int artistid, const QString& name_orig, bool& isnew );

    QList< int > searchTable( const QString& table, const QString& name, uint limit = 10 );
    QList< int > getTrackFids( int tid );

    static QString sortname( const QString& str );

    QVariantMap artist( int id );
    QVariantMap album( int id );
    QVariantMap track( int id );
    QVariantMap file( int fid );
    QVariantMap result( const QString& url );

    static bool scorepairSorter( const QPair<int,float>& left, const QPair<int,float>& right )
    {
        return left.second > right.second;
    }

    // indexes entries from "table" where id >= pkey
    void updateSearchIndex();

    QString dbid() const { return m_dbid; }

    void loadIndex();

signals:
    void indexReady();

public slots:    

private:
    bool updateSchema( int currentver );

    QSqlDatabase db;

    QString m_lastart, m_lastalb, m_lasttrk;
    int m_lastartid, m_lastalbid, m_lasttrkid;

    QString m_dbid;

    FuzzyIndex* m_fuzzyIndex;
};

#endif // DATABASEIMPL_H
