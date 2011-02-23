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
    Q_ASSERT( source()->isLocal() || source()->id() >= 1 );
    TomahawkSqlQuery query = dbi->newquery();

    QVariantMap m;
    if ( source()->isLocal() )
    {
        query.exec( "SELECT count(*), max(mtime), (SELECT guid FROM oplog WHERE source IS NULL ORDER BY id DESC LIMIT 1) "
                    "FROM file "
                    "WHERE source IS NULL" );
    }
    else
    {
        query.prepare( "SELECT count(*), max(mtime), (SELECT lastop FROM source WHERE id = ?) "
                       "FROM file "
                       "WHERE source = ?" );
        query.addBindValue( source()->id() );
        query.addBindValue( source()->id() );
        query.exec();
    }

    if ( query.next() )
    {
        m.insert( "numfiles", query.value( 0 ).toInt() );
        m.insert( "lastmodified", query.value( 1 ).toInt() );

        if ( !source()->isLocal() && !source()->lastOpGuid().isEmpty() )
            m.insert( "lastop", source()->lastOpGuid() );
        else
            m.insert( "lastop", query.value( 2 ).toString() );
    }

    emit done( m );
}
