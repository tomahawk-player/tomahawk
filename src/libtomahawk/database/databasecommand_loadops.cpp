#include "databasecommand_loadops.h"


void
DatabaseCommand_loadOps::exec( DatabaseImpl* dbi )
{
    QList< dbop_ptr > ops;

    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( QString(
                   "SELECT guid, command, json, compressed, singleton "
                   "FROM oplog "
                   "WHERE source %1 "
                   "AND id > coalesce((SELECT id FROM oplog WHERE guid = ?),0) "
                   "ORDER BY id ASC"
                   ).arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) )
                  );
    query.addBindValue( m_since );
    if( !query.exec() )
    {
        Q_ASSERT(0);
    }

    QString lastguid = m_since;
    while( query.next() )
    {
        dbop_ptr op( new DBOp );
        op->guid = query.value( 0 ).toString();
        op->command = query.value( 1 ).toString();
        op->payload = query.value( 2 ).toByteArray();
        op->compressed = query.value( 3 ).toBool();
        op->singleton = query.value( 4 ).toBool();

        lastguid = op->guid;
        ops << op;
    }

    qDebug() << "Loaded" << ops.length() << "ops from db";
    emit done( m_since, lastguid, ops );
}
