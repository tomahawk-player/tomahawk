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

#ifndef DATABASEIMPL_H
#define DATABASEIMPL_H

#include <QObject>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QVariant>
#include <QVariantMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QHash>
#include <QThread>

#include "DllMacro.h"
#include "TomahawkSqlQuery.h"
#include "Typedefs.h"

class Database;
class FuzzyIndex;

class DLLEXPORT DatabaseImpl : public QObject
{
Q_OBJECT

friend class FuzzyIndex;
friend class DatabaseCommand_UpdateSearchIndex;

public:
    DatabaseImpl( const QString& dbname );
    ~DatabaseImpl();

    DatabaseImpl* clone() const;

    TomahawkSqlQuery newquery();
    QSqlDatabase& database();

    int artistId( const QString& name_orig, bool autoCreate ); //also for composers!
    int trackId( int artistid, const QString& name_orig, bool autoCreate );
    int albumId( int artistid, const QString& name_orig, bool autoCreate );

    QList< QPair<int, float> > search( const Tomahawk::query_ptr& query, uint limit = 0 );
    QList< QPair<int, float> > searchAlbum( const Tomahawk::query_ptr& query, uint limit = 0 );
    QList< int > getTrackFids( int tid );

    static QString sortname( const QString& str, bool replaceArticle = false );

    QVariantMap artist( int id );
    QVariantMap album( int id );
    QVariantMap track( int id );
    Tomahawk::result_ptr file( int fid );
    Tomahawk::result_ptr resultFromHint( const Tomahawk::query_ptr& query );

    static bool scorepairSorter( const QPair<int,float>& left, const QPair<int,float>& right )
    {
        return left.second > right.second;
    }

    QString dbid() const { return m_dbid; }

    void loadIndex();

signals:
    void indexReady();

private:
    DatabaseImpl( const QString& dbname, bool internal );
    void setFuzzyIndex( FuzzyIndex* fi ) { m_fuzzyIndex = fi; }
    void setDatabaseID( const QString& dbid ) { m_dbid = dbid; }

    void init();
    bool openDatabase( const QString& dbname, bool checkSchema = true );
    bool updateSchema( int oldVersion );
    void dumpDatabase();
    QString cleanSql( const QString& sql );

    bool m_ready;
    QSqlDatabase m_db;

    QString m_lastart, m_lastalb, m_lasttrk;
    int m_lastartid, m_lastalbid, m_lasttrkid;

    QString m_dbid;
    FuzzyIndex* m_fuzzyIndex;
    mutable QMutex m_mutex;
};

#endif // DATABASEIMPL_H
