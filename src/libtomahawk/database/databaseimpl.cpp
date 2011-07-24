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

#include "databaseimpl.h"

#include <QCoreApplication>
#include <QRegExp>
#include <QStringList>
#include <QtAlgorithms>
#include <QFile>

#include "database/database.h"
#include "databasecommand_updatesearchindex.h"
#include "sourcelist.h"
#include "result.h"
#include "artist.h"
#include "album.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

/* !!!! You need to manually generate schema.sql.h when the schema changes:
    cd src/libtomahawk/database
   ./gen_schema.h.sh ./schema.sql tomahawk > schema.sql.h
*/
#include "schema.sql.h"

#define CURRENT_SCHEMA_VERSION 25


DatabaseImpl::DatabaseImpl( const QString& dbname, Database* parent )
    : QObject( (QObject*) parent )
    , m_lastartid( 0 )
    , m_lastalbid( 0 )
    , m_lasttrkid( 0 )
{
    bool schemaUpdated = false;
    int version = getDatabaseVersion( dbname );

    if ( version > 0 && version != CURRENT_SCHEMA_VERSION )
    {
        QString newname = QString( "%1.v%2" ).arg( dbname ).arg( version );
        qDebug() << endl << "****************************" << endl;
        qDebug() << "Schema version too old: " << version << ". Current version is:" << CURRENT_SCHEMA_VERSION;
        qDebug() << "Moving" << dbname << newname;
        qDebug() << "If the migration fails, you can recover your DB by copying" << newname << "back to" << dbname;
        qDebug() << endl << "****************************" << endl;

        QFile::copy( dbname, newname );
        {
            db = QSqlDatabase::addDatabase( "QSQLITE", "tomahawk" );
            db.setDatabaseName( dbname );
            if( !db.open() )
                throw "db moving failed";

            TomahawkSqlQuery query = newquery();
            query.exec( "PRAGMA auto_vacuum = FULL" );

            schemaUpdated = updateSchema( version );
            if ( !schemaUpdated )
            {
                Q_ASSERT( false );
                QTimer::singleShot( 0, qApp, SLOT( quit() ) );
            }
        }
    }
    else
    {
        db = QSqlDatabase::addDatabase( "QSQLITE", "tomahawk" );
        db.setDatabaseName( dbname );
        if ( !db.open() )
        {
            qDebug() << "Failed to open database" << dbname;
            throw "failed to open db"; // TODO
        }

        if ( version < 0 )
            schemaUpdated = updateSchema( 0 );
    }

    TomahawkSqlQuery query = newquery();
    query.exec( "SELECT v FROM settings WHERE k='dbid'" );
    if( query.next() )
    {
        m_dbid = query.value( 0 ).toString();
    }
    else
    {
        m_dbid = uuid();
        query.exec( QString( "INSERT INTO settings(k,v) VALUES('dbid','%1')" ).arg( m_dbid ) );
    }
    qDebug() << "Database ID:" << m_dbid;

     // make sqlite behave how we want:
    query.exec( "PRAGMA synchronous  = ON" );
    query.exec( "PRAGMA foreign_keys = ON" );
    //query.exec( "PRAGMA temp_store = MEMORY" );

    // in case of unclean shutdown last time:
    query.exec( "UPDATE source SET isonline = 'false'" );

    m_fuzzyIndex = new FuzzyIndex( *this, schemaUpdated );
}


DatabaseImpl::~DatabaseImpl()
{
    delete m_fuzzyIndex;
}


void
DatabaseImpl::loadIndex()
{
    connect( m_fuzzyIndex, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );
    m_fuzzyIndex->loadLuceneIndex();
}


void
DatabaseImpl::updateSearchIndex()
{
    DatabaseCommand* cmd = new DatabaseCommand_UpdateSearchIndex();
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


bool
DatabaseImpl::updateSchema( int oldVersion )
{
    // we are called here with the old database. we must migrate it to the CURRENT_SCHEMA_VERSION from the oldVersion
    if ( oldVersion == 0 ) // empty database, so create our tables and stuff
    {
        qDebug() << "Create tables... old version is" << oldVersion;
        QString sql( get_tomahawk_sql() );
        QStringList statements = sql.split( ";", QString::SkipEmptyParts );
        db.transaction();

        foreach ( const QString& sl, statements )
        {
            QString s( sl.trimmed() );
            if ( s.length() == 0 )
                continue;

            qDebug() << "Executing:" << s;
            TomahawkSqlQuery query = newquery();
            query.exec( s );
        }

        db.commit();
        return true;
    }
    else // update in place! run the proper upgrade script
    {
        int cur = oldVersion;
        db.transaction();
        while ( cur < CURRENT_SCHEMA_VERSION )
        {
            cur++;

            QString path = QString( RESPATH "sql/dbmigrate-%1_to_%2.sql" ).arg( cur - 1 ).arg( cur );
            QFile script( path );
            if ( !script.exists() || !script.open( QIODevice::ReadOnly ) )
            {
                qWarning() << "Failed to find or open upgrade script from" << (cur-1) << "to" << cur << " (" << path << ")! Aborting upgrade...";
                return false;
            }

            QString sql = QString::fromUtf8( script.readAll() ).trimmed();
            QStringList statements = sql.split( ";", QString::SkipEmptyParts );
            foreach ( const QString& sql, statements )
            {
                QString clean = cleanSql( sql ).trimmed();
                if ( clean.isEmpty() )
                    continue;

                qDebug() << "Executing upgrade statement:" << clean;
                TomahawkSqlQuery q = newquery();
                q.exec( clean );
            }
        }
        db.commit();
        qDebug() << "DB Upgrade successful!";
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
    Tomahawk::result_ptr r = Tomahawk::result_ptr( new Tomahawk::Result() );
    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT url, mtime, size, md5, mimetype, duration, bitrate, "
                         "file_join.artist, file_join.album, file_join.track, "
                         "(select name from artist where id = file_join.artist) as artname, "
                         "(select name from album  where id = file_join.album)  as albname, "
                         "(select name from track  where id = file_join.track)  as trkname, "
                         "source "
                         "FROM file, file_join "
                         "WHERE file.id = file_join.file AND file.id = %1" )
                .arg( fid ) );

    if ( query.next() )
    {
        Tomahawk::source_ptr s;

        const QString url_str = query.value( 0 ).toString();
        if ( query.value( 13 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
        }
        else
        {
            s = SourceList::instance()->get( query.value( 13 ).toUInt() );
            if ( s.isNull() )
            {
                return r;
            }

            r->setUrl( QString( "servent://%1\t%2" ).arg( s->userName() ).arg( url_str ) );
        }

        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 7 ).toUInt(), query.value( 10 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( query.value( 8 ).toUInt(), query.value( 11 ).toString(), artist );

        r->setUrl( query.value( 0 ).toString() );
        r->setModificationTime( query.value( 1 ).toUInt() );
        r->setSize( query.value( 2 ).toUInt() );
        r->setMimetype( query.value( 4 ).toString() );
        r->setDuration( query.value( 5 ).toUInt() );
        r->setBitrate( query.value( 6 ).toUInt() );
        r->setArtist( artist );
        r->setAlbum( album );
        r->setTrack( query.value( 12 ).toString() );
        r->setId( query.value( 9 ).toUInt() );
        r->setCollection( s->collection() );
        r->setScore( 1.0 );
    }

    return r;
}


int
DatabaseImpl::artistId( const QString& name_orig, bool& autoCreate )
{
    bool isnew = false;
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
            qDebug() << "Failed to insert artist:" << name_orig;
            return 0;
        }

        id = query.lastInsertId().toInt();
        isnew = true;
        m_lastart = name_orig;
        m_lastartid = id;
    }

    autoCreate = isnew;
    return id;
}


int
DatabaseImpl::trackId( int artistid, const QString& name_orig, bool& isnew )
{
    isnew = false;
    int id = 0;
    QString sortname = DatabaseImpl::sortname( name_orig );
    //if( ( id = m_artistcache[sortname] ) ) return id;

    TomahawkSqlQuery query = newquery();
    query.prepare( "SELECT id FROM track WHERE artist = ? AND sortname = ?" );
    query.addBindValue( artistid );
    query.addBindValue( sortname );
    query.exec();

    if( query.next() )
    {
        id = query.value( 0 ).toInt();
    }
    if( id )
    {
        //m_trackcache[sortname]=id;
        return id;
    }

    // not found, insert it.
    query.prepare( "INSERT INTO track(id,artist,name,sortname) VALUES(NULL,?,?,?)" );
    query.addBindValue( artistid );
    query.addBindValue( name_orig );
    query.addBindValue( sortname );
    if( !query.exec() )
    {
        qDebug() << "Failed to insert track:" << name_orig ;
        return 0;
    }

    id = query.lastInsertId().toInt();
    //m_trackcache[sortname]=id;
    isnew = true;
    return id;
}


int
DatabaseImpl::albumId( int artistid, const QString& name_orig, bool& isnew )
{
    isnew = false;
    if( name_orig.isEmpty() )
    {
        //qDebug() << Q_FUNC_INFO << "empty album name";
        return 0;
    }

    if( m_lastartid == artistid && m_lastalb == name_orig )
        return m_lastalbid;

    int id = 0;
    QString sortname = DatabaseImpl::sortname( name_orig );
    //if( ( id = m_albumcache[sortname] ) ) return id;

    TomahawkSqlQuery query = newquery();
    query.prepare( "SELECT id FROM album WHERE artist = ? AND sortname = ?" );
    query.addBindValue( artistid );
    query.addBindValue( sortname );
    query.exec();
    if( query.next() )
    {
        id = query.value( 0 ).toInt();
    }
    if( id )
    {
        m_lastalb = name_orig;
        m_lastalbid = id;
        return id;
    }

    // not found, insert it.
    query.prepare( "INSERT INTO album(id,artist,name,sortname) VALUES(NULL,?,?,?)" );
    query.addBindValue( artistid );
    query.addBindValue( name_orig );
    query.addBindValue( sortname );
    if( !query.exec() )
    {
        qDebug() << "Failed to insert album: " << name_orig ;
        return 0;
    }

    id = query.lastInsertId().toInt();
    //m_albumcache[sortname]=id;
    isnew = true;
    m_lastalb = name_orig;
    m_lastalbid = id;
    return id;
}


QList< QPair<int, float> >
DatabaseImpl::searchTable( const QString& table, const QString& name, uint limit )
{
    Q_UNUSED( limit );

    QList< QPair<int, float> > resultslist;
    if( table != "artist" && table != "track" && table != "album" )
        return resultslist;

    QMap< int, float > resultsmap = m_fuzzyIndex->search( table, name );
    foreach( int i, resultsmap.keys() )
    {
        resultslist << QPair<int, float>( i, (float)resultsmap.value( i ) );
    }
    qSort( resultslist.begin(), resultslist.end(), DatabaseImpl::scorepairSorter );

    return resultslist;
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
DatabaseImpl::sortname( const QString& str )
{
    return str.toLower().trimmed().replace( QRegExp("[\\s]{2,}"), " " );
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
    else
    {
//        Q_ASSERT( false );
        qDebug() << "We don't support non-servent / non-file result-hints yet.";
/*        res = Tomahawk::result_ptr( new Tomahawk::Result() );
        s = SourceList::instance()->webSource();
        res->setUrl( url );
        res->setCollection( s->collection() );
        res->setRID( uuid() );
        res->setScore( 1.0 );
        res->setArtist( Tomahawk::artist_ptr( new Tomahawk::Artist( 0, origquery->artist() ) ) );
        res->setAlbum( Tomahawk::album_ptr( new Tomahawk::Album( 0, origquery->album(), res->artist() ) ) );
        res->setTrack( origquery->track() );
        res->setDuration( origquery->duration() );
        res->setFriendlySource( url );*/

        return res;
    }

    res = Tomahawk::result_ptr( new Tomahawk::Result() );
    bool searchlocal = s->isLocal();

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, file_join.artist, file_join.album, file_join.track, "
                            "artist.name as artname, "
                            "album.name as albname, "
                            "track.name as trkname, "
                            "file.source, "
                            "file_join.albumpos, "
                            "artist.id as artid, "
                            "album.id as albid, "
                            "track_attributes.v as year "
                            "FROM file, file_join, artist, track, track_attributes "
                            "LEFT JOIN album ON album.id = file_join.album "
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

        const QString url_str = query.value( 0 ).toString();
        if ( query.value( 13 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
            res->setUrl( url_str );
        }
        else
        {
            s = SourceList::instance()->get( query.value( 13 ).toUInt() );
            if ( s.isNull() )
            {
                return res;
            }

            res->setUrl( QString( "servent://%1\t%2" ).arg( s->userName() ).arg( url_str ) );
        }

        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 15 ).toUInt(), query.value( 10 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( query.value( 16 ).toUInt(), query.value( 11 ).toString(), artist );

        res->setModificationTime( query.value( 1 ).toUInt() );
        res->setSize( query.value( 2 ).toUInt() );
        res->setMimetype( query.value( 4 ).toString() );
        res->setDuration( query.value( 5 ).toInt() );
        res->setBitrate( query.value( 6 ).toInt() );
        res->setArtist( artist );
        res->setAlbum( album );
        res->setScore( 1.0 );
        res->setTrack( query.value( 12 ).toString() );
        res->setAlbumPos( query.value( 14 ).toUInt() );
        res->setRID( uuid() );
        res->setId( query.value( 9 ).toUInt() );
        res->setCollection( s->collection() );
        res->setYear( query.value( 17 ).toUInt() );
    }

    return res;
}


int
DatabaseImpl::getDatabaseVersion( const QString& dbname )
{
    int version = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "tomahawk" );
        db.setDatabaseName( dbname );
        if ( !db.open() )
        {
            qDebug() << "Failed to open database" << dbname;
            throw "failed to open db"; // TODO
        }

        QSqlQuery qry = QSqlQuery( db );
        qry.exec( "SELECT v FROM settings WHERE k='schema_version'" );
        if ( qry.next() )
        {
            version = qry.value( 0 ).toInt();
            qDebug() << "Database schema of" << dbname << "is" << version;
        }
    }

    QSqlDatabase::removeDatabase( "tomahawk" );

    return version;
}
