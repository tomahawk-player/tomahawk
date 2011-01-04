#include "databasecommand_sourceoffline.h"


DatabaseCommand_SourceOffline::DatabaseCommand_SourceOffline( int id )
    : DatabaseCommand()
    , m_id( id )
{
}


void DatabaseCommand_SourceOffline::exec( DatabaseImpl* lib )
{
    TomahawkSqlQuery q = lib->newquery();
    q.exec( QString( "UPDATE source SET isonline = 'false' WHERE id = %1" )
            .arg( m_id ) );
}
