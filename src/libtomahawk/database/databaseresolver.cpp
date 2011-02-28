#include "databaseresolver.h"

#include "network/servent.h"
#include "database/database.h"
#include "database/databasecommand_resolve.h"


DatabaseResolver::DatabaseResolver( int weight )
    : Resolver()
    , m_weight( weight )
{
}


void
DatabaseResolver::resolve( const Tomahawk::query_ptr& query )
{
    DatabaseCommand_Resolve* cmd = new DatabaseCommand_Resolve( query );

    connect( cmd, SIGNAL( results( Tomahawk::QID, QList< Tomahawk::result_ptr > ) ),
                    SLOT( gotResults( Tomahawk::QID, QList< Tomahawk::result_ptr > ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

}


void
DatabaseResolver::gotResults( const Tomahawk::QID qid, QList< Tomahawk::result_ptr> results )
{
    qDebug() << Q_FUNC_INFO << qid << results.length();

    Tomahawk::Pipeline::instance()->reportResults( qid, results );
}


QString
DatabaseResolver::name() const
{
    return QString( "DatabaseResolver" );
}
