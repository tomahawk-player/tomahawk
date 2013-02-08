/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DatabaseImpl.h"

#include <QCoreApplication>
#include <QRegExp>
#include <QStringList>
#include <QtAlgorithms>
#include <QFile>

#include "database/Database.h"
#include "FuzzyIndex.h"
#include "SourceList.h"
#include "Result.h"
#include "Artist.h"
#include "Album.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

/* !!!! You need to manually generate Schema.sql.h when the schema changes:
    cd src/libtomahawk/database
   ./gen_schema.h.sh ./Schema.sql tomahawk > Schema.sql.h
*/
#include "Schema.sql.h"

#define CURRENT_SCHEMA_VERSION 29

DatabaseImpl::DatabaseImpl( const QString& dbname )
{
    QTime t;
    t.start();

    bool schemaUpdated = openDatabase( dbname );
    tDebug( LOGVERBOSE ) << "Opened database:" << t.elapsed();

    TomahawkSqlQuery query = newquery();
    query.exec( "SELECT v FROM settings WHERE k='dbid'" );
    if ( query.next() )
    {
        m_dbid = query.value( 0 ).toString();
    }
    else
    {
        m_dbid = uuid();
        query.exec( QString( "INSERT INTO settings(k,v) VALUES('dbid','%1')" ).arg( m_dbid ) );
    }

    tLog() << "Database ID:" << m_dbid;
    init();
    query.exec( "PRAGMA auto_vacuum = FULL" );
    query.exec( "PRAGMA synchronous = NORMAL" );

    tDebug( LOGVERBOSE ) << "Tweaked db pragmas:" << t.elapsed();

    // in case of unclean shutdown last time:
    query.exec( "UPDATE source SET isonline = 'false'" );
    query.exec( "DELETE FROM oplog WHERE source IS NULL AND singleton = 'true'" );

    m_fuzzyIndex = new FuzzyIndex( this, schemaUpdated );

    tDebug( LOGVERBOSE ) << "Loaded index:" << t.elapsed();
    if ( qApp->arguments().contains( "--dumpdb" ) )
    {
        dumpDatabase();
        ::exit( 0 );
    }
}


DatabaseImpl::DatabaseImpl( const QString& dbname, bool internal )
{
    Q_UNUSED( internal );
    openDatabase( dbname, false );
    init();
}


void
DatabaseImpl::init()
{
    m_lastartid = m_lastalbid = m_lasttrkid = 0;

    TomahawkSqlQuery query = newquery();

     // make sqlite behave how we want:
    query.exec( "PRAGMA foreign_keys = ON" );
}


DatabaseImpl::~DatabaseImpl()
{
    tDebug() << "Shutting down database connection.";

/*
#ifdef TOMAHAWK_QUERY_ANALYZE
    TomahawkSqlQuery q = newquery();

    q.exec( "ANALYZE" );
    q.exec( "SELECT * FROM sqlite_stat1" );
    while ( q.next() )
    {
        tLog( LOGSQL ) << q.value( 0 ).toString() << q.value( 1 ).toString() << q.value( 2 ).toString();
    }

#endif
*/
}


TomahawkSqlQuery
DatabaseImpl::newquery()
{
    QMutexLocker lock( &m_mutex );
    return TomahawkSqlQuery( m_db );
}


QSqlDatabase&
DatabaseImpl::database()
{
    QMutexLocker lock( &m_mutex );
    return m_db;
}


DatabaseImpl*
DatabaseImpl::clone() const
{
    QMutexLocker lock( &m_mutex );

    DatabaseImpl* impl = new DatabaseImpl( m_db.databaseName(), true );
    impl->setDatabaseID( m_dbid );
    impl->setFuzzyIndex( m_fuzzyIndex );
    return impl;
}


void
DatabaseImpl::dumpDatabase()
{
    QFile dump( "dbdump.txt" );
    if ( !dump.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        tDebug() << "Couldn't open dbdump.txt for writing!";
        Q_ASSERT( false );
    }
    else
    {
        QTextStream dumpout( &dump );
        TomahawkSqlQuery query = newquery();

        query.exec( "SELECT * FROM oplog" );
        while ( query.next() )
        {
            dumpout << "ID: " << query.value( 0 ).toInt() << endl
                    << "GUID: " << query.value( 2 ).toString() << endl
                    << "Command: " << query.value( 3 ).toString() << endl
                    << "Singleton: " << query.value( 4 ).toBool() << endl
                    << "JSON: " << ( query.value( 5 ).toBool() ? qUncompress( query.value( 6 ).toByteArray() ) : query.value( 6 ).toByteArray() )
                    << endl << endl << endl;
        }
    }
}


void
DatabaseImpl::loadIndex()
{
    connect( m_fuzzyIndex, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );
    m_fuzzyIndex->loadLuceneIndex();
}


bool
DatabaseImpl::updateSchema( int oldVersion )
{
    // we are called here with the old database. we must migrate it to the CURRENT_SCHEMA_VERSION from the oldVersion
    if ( oldVersion == 0 ) // empty database, so create our tables and stuff
    {
        tLog() << "Create tables... old version is" << oldVersion;
        QString sql( get_tomahawk_sql() );
        QStringList statements = sql.split( ";", QString::SkipEmptyParts );
        m_db.transaction();

        foreach ( const QString& sl, statements )
        {
            QString s( sl.trimmed() );
            if ( s.length() == 0 )
                continue;

            tLog() << "Executing:" << s;
            TomahawkSqlQuery query = newquery();
            query.exec( s );
        }

        m_db.commit();
        return true;
    }
    else // update in place! run the proper upgrade script
    {
        int cur = oldVersion;
        m_db.transaction();
        while ( cur < CURRENT_SCHEMA_VERSION )
        {
            cur++;

            QString path = QString( RESPATH "sql/dbmigrate-%1_to_%2.sql" ).arg( cur - 1 ).arg( cur );
            QFile script( path );
            if ( !script.exists() || !script.open( QIODevice::ReadOnly ) )
            {
                tLog() << "Failed to find or open upgrade script from" << (cur-1) << "to" << cur << " (" << path << ")! Aborting upgrade...";
                return false;
            }

            QString sql = QString::fromUtf8( script.readAll() ).trimmed();
            QStringList statements = sql.split( ";", QString::SkipEmptyParts );
            foreach ( const QString& sql, statements )
            {
                QString clean = cleanSql( sql ).trimmed();
                if ( clean.isEmpty() )
                    continue;

                tLog() << "Executing upgrade statement:" << clean;
                TomahawkSqlQuery q = newquery();
                q.exec( clean );
            }
        }
        m_db.commit();
        tLog() << "DB Upgrade successful!";
        return true;
    }
}


QString
DatabaseImpl::cleanSql( const QString& sql )
{
    QString fixed = sql;
    QRegExp r( "--[^\\n]*" );
    fixed.replace( r, QString() );
    return fixed.trimmed();
}


Tomahawk::result_ptr
DatabaseImpl::file( int fid )
{
    Tomahawk::result_ptr r;
    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT url, mtime, size, md5, mimetype, duration, bitrate, "
                         "file_join.artist, file_join.album, file_join.track, file_join.composer, "
                         "(select name from artist where id = file_join.artist) as artname, "
                         "(select name from album  where id = file_join.album)  as albname, "
                         "(select name from track  where id = file_join.track)  as trkname, "
                         "(select name from artist where id = file_join.composer) as cmpname, "
                         "source "
                         "FROM file, file_join "
                         "WHERE file.id = file_join.file AND file.id = %1" )
                .arg( fid ) );

    if ( query.next() )
    {
        Tomahawk::source_ptr s;
        QString url = query.value( 0 ).toString();

        if ( query.value( 15 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
        }
        else
        {
            s = SourceList::instance()->get( query.value( 15 ).toUInt() );
            if ( s.isNull() )
            {
                return r;
            }

            url = QString( "servent://%1\t%2" ).arg( s->nodeId() ).arg( url );
        }

        r = Tomahawk::Result::get( url );
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 7 ).toUInt(), query.value( 11 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( query.value( 8 ).toUInt(), query.value( 12 ).toString(), artist );
        Tomahawk::artist_ptr composer = Tomahawk::Artist::get( query.value( 10 ).toUInt(), query.value( 14 ).toString() );

        r->setModificationTime( query.value( 1 ).toUInt() );
        r->setSize( query.value( 2 ).toUInt() );
        r->setMimetype( query.value( 4 ).toString() );
        r->setDuration( query.value( 5 ).toUInt() );
        r->setBitrate( query.value( 6 ).toUInt() );
        r->setArtist( artist );
        r->setAlbum( album );
        r->setComposer( composer );
        r->setTrack( query.value( 13 ).toString() );
        r->setTrackId( query.value( 9 ).toUInt() );
        r->setCollection( s->dbCollection() );
        r->setScore( 1.0 );
        r->setFileId( fid );
    }

    return r;
}


int
DatabaseImpl::artistId( const QString& name_orig, bool autoCreate )
{
    if ( m_lastart == name_orig )
        return m_lastartid;

    int id = 0;
    QString sortname = DatabaseImpl::sortname( name_orig );

    TomahawkSqlQuery query = newquery();
    query.prepare( "SELECT id FROM artist WHERE sortname = ?" );
    query.addBindValue( sortname );
    query.exec();
    if ( query.next() )
    {
        id = query.value( 0 ).toInt();
    }
    if ( id )
    {
        m_lastart = name_orig;
        m_lastartid = id;
        return id;
    }

    if ( autoCreate )
    {
        // not found, insert it.
        query.prepare( "INSERT INTO artist(id,name,sortname) VALUES(NULL,?,?)" );
        query.addBindValue( name_orig );
        query.addBindValue( sortname );
        if ( !query.exec() )
        {
            tDebug() << "Failed to insert artist:" << name_orig;
            return 0;
        }

        id = query.lastInsertId().toInt();
        m_lastart = name_orig;
        m_lastartid = id;
    }

    return id;
}


int
DatabaseImpl::trackId( int artistid, const QString& name_orig, bool autoCreate )
{
    int id = 0;
    QString sortname = DatabaseImpl::sortname( name_orig );
    //if( ( id = m_artistcache[sortname] ) ) return id;

    TomahawkSqlQuery query = newquery();
    query.prepare( "SELECT id FROM track WHERE artist = ? AND sortname = ?" );
    query.addBindValue( artistid );
    query.addBindValue( sortname );
    query.exec();

    if ( query.next() )
    {
        id = query.value( 0 ).toInt();
    }
    if ( id )
    {
        //m_trackcache[sortname]=id;
        return id;
    }

    if ( autoCreate )
    {
        // not found, insert it.
        query.prepare( "INSERT INTO track(id,artist,name,sortname) VALUES(NULL,?,?,?)" );
        query.addBindValue( artistid );
        query.addBindValue( name_orig );
        query.addBindValue( sortname );
        if ( !query.exec() )
        {
            tDebug() << "Failed to insert track:" << name_orig;
            return 0;
        }

        id = query.lastInsertId().toInt();
    }

    return id;
}


int
DatabaseImpl::albumId( int artistid, const QString& name_orig, bool autoCreate )
{
    if ( name_orig.isEmpty() )
    {
        //qDebug() << Q_FUNC_INFO << "empty album name";
        return 0;
    }

    if ( m_lastartid == artistid && m_lastalb == name_orig )
        return m_lastalbid;

    int id = 0;
    QString sortname = DatabaseImpl::sortname( name_orig );
    //if( ( id = m_albumcache[sortname] ) ) return id;

    TomahawkSqlQuery query = newquery();
    query.prepare( "SELECT id FROM album WHERE artist = ? AND sortname = ?" );
    query.addBindValue( artistid );
    query.addBindValue( sortname );
    query.exec();
    if ( query.next() )
    {
        id = query.value( 0 ).toInt();
    }
    if ( id )
    {
        m_lastalb = name_orig;
        m_lastalbid = id;
        return id;
    }

    if ( autoCreate )
    {
        // not found, insert it.
        query.prepare( "INSERT INTO album(id,artist,name,sortname) VALUES(NULL,?,?,?)" );
        query.addBindValue( artistid );
        query.addBindValue( name_orig );
        query.addBindValue( sortname );
        if( !query.exec() )
        {
            tDebug() << "Failed to insert album:" << name_orig;
            return 0;
        }

        id = query.lastInsertId().toInt();
        m_lastalb = name_orig;
        m_lastalbid = id;
    }

    return id;
}


QList< QPair<int, float> >
DatabaseImpl::search( const Tomahawk::query_ptr& query, uint limit )
{
    QList< QPair<int, float> > resultslist;

    QMap< int, float > resultsmap = m_fuzzyIndex->search( query );
    foreach ( int i, resultsmap.keys() )
    {
        resultslist << QPair<int, float>( i, (float)resultsmap.value( i ) );
    }
    qSort( resultslist.begin(), resultslist.end(), DatabaseImpl::scorepairSorter );

    if ( !limit )
        return resultslist;

    QList< QPair<int, float> > resultscapped;
    for ( int i = 0; i < (int)limit && i < resultsmap.count(); i++ )
    {
        resultscapped << resultslist.at( i );
    }

    return resultscapped;
}


QList< QPair<int, float> >
DatabaseImpl::searchAlbum( const Tomahawk::query_ptr& query, uint limit )
{
    QList< QPair<int, float> > resultslist;

    QMap< int, float > resultsmap = m_fuzzyIndex->searchAlbum( query );
    foreach ( int i, resultsmap.keys() )
    {
        resultslist << QPair<int, float>( i, (float)resultsmap.value( i ) );
    }
    qSort( resultslist.begin(), resultslist.end(), DatabaseImpl::scorepairSorter );

    if ( !limit )
        return resultslist;

    QList< QPair<int, float> > resultscapped;
    for ( int i = 0; i < (int)limit && i < resultsmap.count(); i++ )
    {
        resultscapped << resultslist.at( i );
    }

    return resultscapped;
}


QList< int >
DatabaseImpl::getTrackFids( int tid )
{
    QList< int > ret;

    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT file.id FROM file, file_join "
                         "WHERE file_join.file=file.id "
                         "AND file_join.track = %1 ").arg( tid ) );
    query.exec();

    while( query.next() )
        ret.append( query.value( 0 ).toInt() );

    return ret;
}


QString
DatabaseImpl::sortname( const QString& str, bool replaceArticle )
{
    QString s = str.toLower().trimmed().replace( QRegExp( "[\\s]{2,}" ), " " );

    if ( replaceArticle && s.startsWith( "the " ) )
    {
        s = s.mid( 4 );
    }

    return s;
}


QVariantMap
DatabaseImpl::artist( int id )
{
    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT id, name, sortname FROM artist WHERE id = %1" ).arg( id ) );

    QVariantMap m;
    if( !query.next() )
        return m;

    m["id"] = query.value( 0 );
    m["name"] = query.value( 1 );
    m["sortname"] = query.value( 2 );
    return m;
}


QVariantMap
DatabaseImpl::track( int id )
{
    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT id, artist, name, sortname FROM track WHERE id = %1" ).arg( id ) );

    QVariantMap m;
    if( !query.next() )
        return m;

    m["id"] = query.value( 0 );
    m["artist"] = query.value( 1 );
    m["name"] = query.value( 2 );
    m["sortname"] = query.value( 3 );
    return m;
}


QVariantMap
DatabaseImpl::album( int id )
{
    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT id, artist, name, sortname FROM album WHERE id = %1" ).arg( id ) );

    QVariantMap m;
    if( !query.next() )
        return m;

    m["id"] = query.value( 0 );
    m["artist"] = query.value( 1 );
    m["name"] = query.value( 2 );
    m["sortname"] = query.value( 3 );
    return m;
}


Tomahawk::result_ptr
DatabaseImpl::resultFromHint( const Tomahawk::query_ptr& origquery )
{
    QString url = origquery->resultHint();
    TomahawkSqlQuery query = newquery();
    Tomahawk::source_ptr s;
    Tomahawk::result_ptr res;
    QString fileUrl;

    if ( url.contains( "servent://" ) )
    {
        QStringList parts = url.mid( QString( "servent://" ).length() ).split( "\t" );
        s = SourceList::instance()->get( parts.at( 0 ) );
        fileUrl = parts.at( 1 );

        if ( s.isNull() )
            return res;
    }
    else if ( url.contains( "file://" ) )
    {
        s = SourceList::instance()->getLocal();
        fileUrl = url;
    }
    else if ( TomahawkUtils::whitelistedHttpResultHint( url ) )
    {
        // Return http resulthint directly
        res = Tomahawk::Result::get( url );
        res->setRID( uuid() );
        res->setScore( 1.0 );
        res->setArtist( Tomahawk::Artist::get( artistId( origquery->artist(), false ), origquery->artist() ) );
        res->setAlbum( Tomahawk::Album::get( albumId( res->artist()->id(), origquery->album(), false ), origquery->album(), res->artist() ) );
        res->setTrack( origquery->track() );
        res->setDuration( origquery->duration() );
        const QUrl u = QUrl::fromUserInput( url );
        res->setFriendlySource( u.host() );

        return res;
    }
    else
    {
        // No resulthint
        return res;
    }

    bool searchlocal = s->isLocal();

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, "  //0
                            "file_join.artist, file_join.album, file_join.track, "  //7
                            "file_join.composer, "                                  //10
                            "artist.name as artname, "                              //11
                            "album.name as albname, "                               //12
                            "track.name as trkname, "                               //13
                            "composer.name as cmpname, "                            //14
                            "file.source, "                                         //15
                            "file_join.albumpos, "                                  //16
                            "file_join.discnumber, "                                //17
                            "artist.id as artid, "                                  //18
                            "album.id as albid, "                                   //19
                            "composer.id as cmpid, "                                //20
                            "track_attributes.v as year "                           //21
                            "FROM file, file_join, artist, track, track_attributes "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "LEFT JOIN artist AS composer on composer.id = file_join.composer "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.source %1 AND "
                            "file_join.file = file.id AND "
                            "file.id = track_attributes.id AND "
                            "file.url = ?"
        ).arg( searchlocal ? "IS NULL" : QString( "= %1" ).arg( s->id() ) );

    query.prepare( sql );
    query.bindValue( 0, fileUrl );
    query.exec();

    if( query.next() )
    {
        Tomahawk::source_ptr s;
        QString url = query.value( 0 ).toString();

        if ( query.value( 15 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
        }
        else
        {
            s = SourceList::instance()->get( query.value( 15 ).toUInt() );
            if ( s.isNull() )
            {
                return res;
            }

            url = QString( "servent://%1\t%2" ).arg( s->nodeId() ).arg( url );
        }

        res = Tomahawk::Result::get( url );
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 18 ).toUInt(), query.value( 11 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( query.value( 19 ).toUInt(), query.value( 12 ).toString(), artist );
        Tomahawk::artist_ptr composer = Tomahawk::Artist::get( query.value( 20 ).toUInt(), query.value( 14 ).toString() );

        res->setModificationTime( query.value( 1 ).toUInt() );
        res->setSize( query.value( 2 ).toUInt() );
        res->setMimetype( query.value( 4 ).toString() );
        res->setDuration( query.value( 5 ).toInt() );
        res->setBitrate( query.value( 6 ).toInt() );
        res->setArtist( artist );
        res->setAlbum( album );
        res->setComposer( composer );
        res->setScore( 1.0 );
        res->setTrack( query.value( 13 ).toString() );
        res->setAlbumPos( query.value( 16 ).toUInt() );
        res->setDiscNumber( query.value( 17 ).toUInt() );
        res->setRID( uuid() );
        res->setTrackId( query.value( 9 ).toUInt() );
        res->setCollection( s->dbCollection() );
        res->setYear( query.value( 21 ).toUInt() );
    }

    return res;
}


bool
DatabaseImpl::openDatabase( const QString& dbname, bool checkSchema )
{
    QString connName( "tomahawk" );
    if ( !checkSchema )
    {
        // secondary connection, use a unique connection name
        connName += "_" + uuid();
    }

    bool schemaUpdated = false;
    int version = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", connName );
        db.setDatabaseName( dbname );
        db.setConnectOptions( "QSQLITE_ENABLE_SHARED_CACHE=1" );
        if ( !db.open() )
        {
            tLog() << "Failed to open database" << dbname;
            throw "failed to open db"; // TODO
        }

        if ( checkSchema )
        {
            QSqlQuery qry = QSqlQuery( db );
            qry.exec( "SELECT v FROM settings WHERE k='schema_version'" );
            if ( qry.next() )
            {
                version = qry.value( 0 ).toInt();
                tLog() << "Database schema of" << dbname << "is" << version;
            }
        }
        else
            version = CURRENT_SCHEMA_VERSION;

        if ( version < 0 || version == CURRENT_SCHEMA_VERSION )
            m_db = db;
    }

    if ( version > 0 && version != CURRENT_SCHEMA_VERSION )
    {
        QSqlDatabase::removeDatabase( connName );

        QString newname = QString( "%1.v%2" ).arg( dbname ).arg( version );
        tLog() << endl << "****************************" << endl;
        tLog() << "Schema version too old: " << version << ". Current version is:" << CURRENT_SCHEMA_VERSION;
        tLog() << "Moving" << dbname << newname;
        tLog() << "If the migration fails, you can recover your DB by copying" << newname << "back to" << dbname;
        tLog() << endl << "****************************" << endl;

        QFile::copy( dbname, newname );
        {
            m_db = QSqlDatabase::addDatabase( "QSQLITE", connName );
            m_db.setDatabaseName( dbname );
            if ( !m_db.open() )
                throw "db moving failed";

            schemaUpdated = updateSchema( version );
            if ( !schemaUpdated )
            {
                Q_ASSERT( false );
                QTimer::singleShot( 0, qApp, SLOT( quit() ) );
            }
        }
    }
    else if ( version < 0 )
    {
            schemaUpdated = updateSchema( 0 );
    }

    return schemaUpdated;
}
