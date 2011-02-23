#include "databasecommand_loadallsources.h"

#include <QSqlQuery>

#include "network/servent.h"
#include "source.h"
#include "databaseimpl.h"

using namespace Tomahawk;


void DatabaseCommand_LoadAllSources::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();

    query.exec( QString( "SELECT id, name, friendlyname "
                         "FROM source" ) );

    QList<source_ptr> sources;
    while ( query.next() )
    {
        source_ptr src( new Source( query.value( 0 ).toUInt(), query.value( 1 ).toString() ) );
        src->setFriendlyName( query.value( 2 ).toString() );
        sources << src;
    }

    emit done( sources );
}

