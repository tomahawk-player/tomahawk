#include "databasecommand_updatesearchindex.h"


DatabaseCommand_UpdateSearchIndex::DatabaseCommand_UpdateSearchIndex()
    : DatabaseCommand()
{
}


void
DatabaseCommand_UpdateSearchIndex::indexTable( DatabaseImpl* db, const QString& table )
{
    qDebug() << Q_FUNC_INFO;
    
    TomahawkSqlQuery query = db->newquery();
    qDebug() << "Building index for" << table;
    query.exec( QString( "SELECT id, name FROM %1" ).arg( table ) );
    
    QMap< unsigned int, QString > fields;
    while ( query.next() )
    {
        fields.insert( query.value( 0 ).toUInt(), query.value( 1 ).toString() );
    }
    
    db->m_fuzzyIndex->appendFields( table, fields );
}


void
DatabaseCommand_UpdateSearchIndex::exec( DatabaseImpl* db )
{
    db->m_fuzzyIndex->beginIndexing();
    
    indexTable( db, "artist" );
    indexTable( db, "album" );
    indexTable( db, "track" );

    db->m_fuzzyIndex->endIndexing();
}
