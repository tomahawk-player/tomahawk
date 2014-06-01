#include "DatabaseFuzzyIndex.h"

#include "database/DatabaseImpl.h"
#include "database/Database.h"

namespace Tomahawk {

DatabaseFuzzyIndex::DatabaseFuzzyIndex( QObject* parent, bool wipe )
    : FuzzyIndex( parent, "tomahawk.lucene", wipe )
{

}


void
DatabaseFuzzyIndex::updateIndex()
{
    Tomahawk::DatabaseCommand* cmd = new Tomahawk::DatabaseCommand_UpdateSearchIndex();
    Tomahawk::Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}

} // namespace Tomahawk
