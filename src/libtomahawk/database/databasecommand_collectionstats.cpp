#include "databasecommand_collectionstats.h"

#include "databaseimpl.h"

using namespace Tomahawk;


DatabaseCommand_CollectionStats::DatabaseCommand_CollectionStats( const source_ptr& source, QObject* parent )
    : DatabaseCommand( source, parent )
{
}


void
DatabaseCommand_CollectionStats::exec( DatabaseImpl* dbi )
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );
    TomahawkSqlQuery query = dbi->newquery();

    Q_ASSERT( source()->isLocal() || source()->id() >= 1 );

    if( source()->isLocal() )
    {
        query.exec("SELECT count(*), max(mtime), (SELECT guid FROM oplog WHERE source IS NULL ORDER BY id DESC LIMIT 1) "
                   "FROM file "
                   "WHERE source IS NULL");
    }
    else
    {
        query.prepare("SELECT count(*), max(mtime), "
                      "       (SELECT lastop FROM source WHERE id = ?) "
                      "FROM file "
                      "WHERE source = ?"
                      );
        query.addBindValue( source()->id() );
        query.addBindValue( source()->id() );
    }
    if( !query.exec() )
    {
        qDebug() << "Failed to get collection stats:" << query.boundValues();
        throw "failed to get collection stats";
    }

    QVariantMap m;
    if( query.next() )
    {
        m.insert( "numfiles", query.value( 0 ).toInt() );
        m.insert( "lastmodified", query.value( 1 ).toInt() );
        m.insert( "lastop", query.value( 2 ).toString() );
    }

    //qDebug() << "Loaded collection stats for"
    //         << (source()->isLocal() ? "LOCAL" : source()->username())
    //         << m;
    emit done( m );
}
