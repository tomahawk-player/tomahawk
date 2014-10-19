/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

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
