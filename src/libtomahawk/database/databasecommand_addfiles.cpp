#include "databasecommand_addfiles.h"

#include <QSqlQuery>

#include "collection.h"
#include "database/database.h"
#include "databasecommand_collectionstats.h"
#include "databaseimpl.h"
#include "network/controlconnection.h"

using namespace Tomahawk;


// remove file paths when making oplog/for network transmission
QVariantList
DatabaseCommand_AddFiles::files() const
{
    QVariantList list;
    foreach ( const QVariant& v, m_files )
    {
        // replace url with the id, we don't leak file paths over the network.
        QVariantMap m = v.toMap();
        m.remove( "url" );
        m.insert( "url", QString::number( m.value( "id" ).toInt() ) );
        list.append( m );
    }
    return list;
}


// After changing a collection, we need to tell other bits of the system:
void
DatabaseCommand_AddFiles::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    if ( source().isNull() || source()->collection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }

    // make the collection object emit its tracksAdded signal, so the
    // collection browser will update/fade in etc.
    Collection* coll = source()->collection().data();

    connect( this, SIGNAL( notify( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
             coll, SLOT( setTracks( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
             Qt::QueuedConnection );

    emit notify( m_queries, source()->collection() );

    // also re-calc the collection stats, to updates the "X tracks" in the sidebar etc:
    DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( source() );
    connect( cmd,            SIGNAL( done( QVariantMap ) ),
             source().data(),  SLOT( setStats( QVariantMap ) ), Qt::QueuedConnection );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_AddFiles::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    TomahawkSqlQuery query_file = dbi->newquery();
    TomahawkSqlQuery query_filejoin = dbi->newquery();
    TomahawkSqlQuery query_trackattr = dbi->newquery();
    TomahawkSqlQuery query_file_del = dbi->newquery();

    query_file.prepare( "INSERT INTO file(source, url, size, mtime, md5, mimetype, duration, bitrate) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)" );
    query_filejoin.prepare( "INSERT INTO file_join(file, artist, album, track, albumpos) "
                            "VALUES (?,?,?,?,?)" );
    query_trackattr.prepare( "INSERT INTO track_attributes(id, k, v) "
                             "VALUES (?,?,?)" );
    query_file_del.prepare( QString( "DELETE FROM file WHERE source %1 AND url = ?" )
                               .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );

    int added = 0;
    QVariant srcid = source()->isLocal() ?
                     QVariant( QVariant::Int ) : source()->id();

    qDebug() << "Adding" << m_files.length() << "files to db for source" << srcid;

    QList<QVariant>::iterator it;
    for( it = m_files.begin(); it != m_files.end(); ++it )
    {
        QVariant& v = *it;
        QVariantMap m = v.toMap();

        QString url         = m.value( "url" ).toString();
        int mtime           = m.value( "mtime" ).toInt();
        int size            = m.value( "size" ).toInt();
        QString hash        = m.value( "hash" ).toString();
        QString mimetype    = m.value( "mimetype" ).toString();
        int duration        = m.value( "duration" ).toInt();
        int bitrate         = m.value( "bitrate" ).toInt();
        QString artist      = m.value( "artist" ).toString();
        QString album       = m.value( "album" ).toString();
        QString track       = m.value( "track" ).toString();
        int albumpos        = m.value( "albumpos" ).toInt();
        int year            = m.value( "year" ).toInt();

        int fileid = 0;
        query_file_del.bindValue( 0, url );
        query_file_del.exec();

        query_file.bindValue( 0, srcid );
        query_file.bindValue( 1, url );
        query_file.bindValue( 2, size );
        query_file.bindValue( 3, mtime );
        query_file.bindValue( 4, hash );
        query_file.bindValue( 5, mimetype );
        query_file.bindValue( 6, duration );
        query_file.bindValue( 7, bitrate );
        if( !query_file.exec() )
        {
            qDebug() << "Failed to insert to file:"
                     << query_file.lastError().databaseText()
                     << query_file.lastError().driverText()
                     << query_file.boundValues();
            continue;
        }
        else
        {
            if( added % 100 == 0 )
                qDebug() << "Inserted" << added;
        }
        // get internal IDs for art/alb/trk
        fileid = query_file.lastInsertId().toInt();

        // insert the new fileid, set the url for our use:
        m.insert( "id", fileid );
        if( !source()->isLocal() )
            m["url"] = QString( "servent://%1\t%2" )
                          .arg( source()->userName() )
                          .arg( fileid );
        v = m;

        bool isnew;
        m["artistid"] = dbi->artistId( artist, isnew );
        if( m["artistid"].toInt() < 1 ) continue;

        m["trackid"] = dbi->trackId( m["artistid"].toInt(), track, isnew );
        if( m["trackid"].toInt() < 1 ) continue;

        m["albumid"] = dbi->albumId( m["artistid"].toInt(), album, isnew );

        // Now add the association
        query_filejoin.bindValue( 0, fileid );
        query_filejoin.bindValue( 1, m["artistid"].toInt() );
        query_filejoin.bindValue( 2, m["albumid"].toInt() > 0 ? m["albumid"].toInt() : QVariant( QVariant::Int ) );
        query_filejoin.bindValue( 3, m["trackid"].toInt() );
        query_filejoin.bindValue( 4, albumpos );
        if( !query_filejoin.exec() )
        {
            qDebug() << "Error inserting into file_join table";
            continue;
        }

        query_trackattr.bindValue( 0, m["trackid"].toInt() );
        query_trackattr.bindValue( 1, "releaseyear" );
        query_trackattr.bindValue( 2, year );
        query_trackattr.exec();

        QVariantMap attr;
        Tomahawk::query_ptr query = Tomahawk::Query::get( m, false );
        m["score"] = 1.0;
        attr["releaseyear"] = m.value( "year" );

        Tomahawk::result_ptr result = Tomahawk::result_ptr( new Tomahawk::Result( m, source()->collection() ) );
        result->setAttributes( attr );

        QList<Tomahawk::result_ptr> results;
        results << result;
        query->addResults( results );

        m_queries << query;

        added++;
    }
    qDebug() << "Inserted" << added;

    // TODO building the index could be a separate job, outside this transaction
    if ( added )
        dbi->updateSearchIndex();

    qDebug() << "Committing" << added << "tracks...";
    emit done( m_files, source()->collection() );
}
