#include "databaseimpl.h"

#include <QCoreApplication>
#include <QRegExp>
#include <QStringList>
#include <QtAlgorithms>
#include <QFile>

#include "database/database.h"
#include "databasecommand_updatesearchindex.h"

/* !!!! You need to manually generate schema.sql.h when the schema changes:
    cd src/libtomahawk/database
   ./gen_schema.h.sh ./schema.sql tomahawk > schema.sql.h
*/
#include "schema.sql.h"

#define CURRENT_SCHEMA_VERSION 16


DatabaseImpl::DatabaseImpl( const QString& dbname, Database* parent )
    : QObject( (QObject*) parent )
    , m_lastartid( 0 )
    , m_lastalbid( 0 )
    , m_lasttrkid( 0 )
    , m_fuzzyIndex( *this )
{
    connect( this, SIGNAL(indexReady()), parent, SIGNAL(indexReady()) );

    db = QSqlDatabase::addDatabase( "QSQLITE", "tomahawk" );
    db.setDatabaseName( dbname );
    if ( !db.open() )
    {
        qDebug() << "FAILED TO OPEN DB";
        throw "failed to open db"; // TODO
    }

    QSqlQuery qry = QSqlQuery( db );
    TomahawkSqlQuery query = newquery();

    qry.exec( "SELECT v FROM settings WHERE k='schema_version'" );
    if ( qry.next() )
    {
        int v = qry.value( 0 ).toInt();
        qDebug() << "Current schema is" << v << this->thread();
        if ( v != CURRENT_SCHEMA_VERSION )
        {  

            QString newname = QString("%1.v%2").arg(dbname).arg(v);
            qDebug() << endl << "****************************" << endl;
            qDebug() << "Schema version too old: " << v << ". Current version is:" << CURRENT_SCHEMA_VERSION;
            qDebug() << "Moving" << dbname << newname;
            qDebug() << endl << "****************************" << endl;

            qry.clear();
            query.clear();
            qry.finish();
            query.finish();
            
            db.close();
            db.removeDatabase( "tomahawk" );

            if( QFile::rename( dbname, newname ) )
            {
                db = QSqlDatabase::addDatabase( "QSQLITE", "tomahawk" );
                db.setDatabaseName( dbname );
                if( !db.open() ) throw "db moving failed";
                updateSchema( v );
            }
            else
            {
                Q_ASSERT(0);
                QTimer::singleShot( 0, qApp, SLOT( quit() ) );
                return;
            }
        }
    } else {
        updateSchema( 0 );
    }

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
}


DatabaseImpl::~DatabaseImpl()
{
    m_indexThread.quit();
    m_indexThread.wait( 5000 );
}


void
DatabaseImpl::loadIndex()
{
    // load ngram index in the background
    m_fuzzyIndex.moveToThread( &m_indexThread );
    connect( &m_indexThread, SIGNAL(started()), &m_fuzzyIndex, SLOT(loadNgramIndex()) );
    connect( &m_fuzzyIndex, SIGNAL(indexReady()), this, SIGNAL(indexReady()) );
    m_indexThread.start();
}


void
DatabaseImpl::updateSearchIndex( const QString& table, int pkey )
{
    DatabaseCommand* cmd = new DatabaseCommand_UpdateSearchIndex(table, pkey);
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


bool
DatabaseImpl::updateSchema( int currentver )
{
    qDebug() << "Create tables... old version is" << currentver;
    QString sql( get_tomahawk_sql() );
    QStringList statements = sql.split( ";", QString::SkipEmptyParts );
    db.transaction();

    foreach( const QString& sl, statements )
    {
        QString s( sl.trimmed() );
        if( s.length() == 0 )
            continue;

        qDebug() << "Executing:" << s;
        TomahawkSqlQuery query;
        query.exec( s );
    }

    db.commit();
    return true;
}


QVariantMap
DatabaseImpl::file( int fid )
{
    QVariantMap m;
    TomahawkSqlQuery query = newquery();
    query.exec( QString( "SELECT url, mtime, size, md5, mimetype, duration, bitrate, "
                         "file_join.artist, file_join.album, file_join.track, "
                         "(select name from artist where id = file_join.artist) as artname, "
                         "(select name from album  where id = file_join.album)  as albname, "
                         "(select name from track  where id = file_join.track)  as trkname "
                         "FROM file, file_join "
                         "WHERE file.id = file_join.file AND file.id = %1" )
                .arg( fid ) );

    if( query.next() )
    {
        m["url"]      = query.value( 0 ).toString();
        m["mtime"]    = query.value( 1 ).toString();
        m["size"]     = query.value( 2 ).toInt();
        m["hash"]     = query.value( 3 ).toString();
        m["mimetype"] = query.value( 4 ).toString();
        m["duration"] = query.value( 5 ).toInt();
        m["bitrate"]  = query.value( 6 ).toInt();
        m["artistid"] = query.value( 7 ).toInt();
        m["albumid"]  = query.value( 8 ).toInt();
        m["trackid"]  = query.value( 8 ).toInt();
        m["artist"]   = query.value( 10 ).toString();
        m["album"]    = query.value( 11 ).toString();
        m["track"]    = query.value( 12 ).toString();
    }

    //qDebug() << m;
    return m;
}


int
DatabaseImpl::artistId( const QString& name_orig, bool& isnew )
{
    isnew = false;
    if( m_lastart == name_orig )
        return m_lastartid;

    int id = 0;
    QString sortname = DatabaseImpl::sortname( name_orig );

    TomahawkSqlQuery query = newquery();
    query.prepare( "SELECT id FROM artist WHERE sortname = ?" );
    query.addBindValue( sortname );
    query.exec();
    if( query.next() )
    {
        id = query.value( 0 ).toInt();
    }
    if( id )
    {
        m_lastart = name_orig;
        m_lastartid = id;
        return id;
    }

    // not found, insert it.
    query.prepare( "INSERT INTO artist(id,name,sortname) VALUES(NULL,?,?)" );
    query.addBindValue( name_orig );
    query.addBindValue( sortname );
    if( !query.exec() )
    {
        qDebug() << "Failed to insert artist:" << name_orig;
        return 0;
    }

    id = query.lastInsertId().toInt();
    isnew = true;
    m_lastart = name_orig;
    m_lastartid = id;
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


QList< int >
DatabaseImpl::searchTable( const QString& table, const QString& name_orig, uint limit )
{
    QList< int > results;
    if( table != "artist" && table != "track" && table != "album" )
        return results;

    QString name = sortname( name_orig );

    // first check for exact matches:
    TomahawkSqlQuery query = newquery();
    query.prepare( QString( "SELECT id FROM %1 WHERE sortname = ?" ).arg( table ) );
    query.addBindValue( name );
    query.exec();

    while( query.next() )
        results.append( query.value( 0 ).toInt() );

    // ngram stuff only works on tracks over a certain length atm:
    if( name_orig.length() > 3 )
    {
        // consult ngram index to find candidates:        
        QMap< int, float > resultsmap = m_fuzzyIndex.search( table, name );

        //qDebug() << "results map for" << table << resultsmap.size();
        QList< QPair<int,float> > resultslist;
        foreach( int i, resultsmap.keys() )
        {
            resultslist << QPair<int,float>( i, (float)resultsmap.value( i ) );
        }
        qSort( resultslist.begin(), resultslist.end(), DatabaseImpl::scorepairSorter );

        for( int k = 0; k < resultslist.size() && k < (int)limit; ++k )
        {
            results << resultslist.at( k ).first;
        }
    }

    return results;
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


QMap< QString, int >
DatabaseImpl::ngrams( const QString& str_orig )
{
    static QMap< QString, QMap<QString, int> > memo;
    if( memo.contains( str_orig ) )
        return memo.value( str_orig );

    int n = 3;
    QMap<QString, int> ret;
    QString str( " " + DatabaseImpl::sortname( str_orig ) + " " );
    int num = str.length();
    QString ngram;

    for( int j = 0; j < num - ( n - 1 ); j++ )
    {
         ngram = str.mid( j, n );
         Q_ASSERT( ngram.length() == n );

         if( ret.contains( ngram ) )
             ret[ngram]++;
         else
             ret[ngram] = 1;
    }

    memo.insert( str_orig, ret );
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

