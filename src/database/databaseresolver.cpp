#include "databaseresolver.h"

#include "tomahawk/tomahawkapp.h"
#include "database.h"
#include "database/databasecommand_resolve.h"

DatabaseResolver::DatabaseResolver( bool searchlocal, int weight )
    : Resolver()
    , m_searchlocal( searchlocal )
    , m_weight( weight )
{
}


void
DatabaseResolver::resolve( QVariant v )
{
    //qDebug() << Q_FUNC_INFO << v;

    if( !m_searchlocal )
    {
        if( APP->servent().numConnectedPeers() == 0 )
            return;
    }

    DatabaseCommand_Resolve* cmd = new DatabaseCommand_Resolve( v, m_searchlocal );

    connect( cmd, SIGNAL( results( Tomahawk::QID, QList< Tomahawk::result_ptr> ) ),
                    SLOT( gotResults( Tomahawk::QID, QList< Tomahawk::result_ptr> ) ), Qt::QueuedConnection );

    APP->database()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

}


void
DatabaseResolver::gotResults( const Tomahawk::QID qid, QList< Tomahawk::result_ptr> results )
{
//    qDebug() << Q_FUNC_INFO << qid << results.length();

    APP->pipeline()->reportResults( qid, results );
}


QString
DatabaseResolver::name() const
{
    return QString( "Database (%1)" ).arg( m_searchlocal ? "local" : "remote" );
}
