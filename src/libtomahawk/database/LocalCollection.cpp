/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "LocalCollection.h"

#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"

#include "PlaylistEntry.h"
#include "SourceList.h"
#include "ViewManager.h"
#include <TomahawkSettings.h>


using namespace Tomahawk;

LocalCollection::LocalCollection( const Tomahawk::source_ptr& source, QObject* parent )
    : DatabaseCollection( source, parent )
{
}


QString
LocalCollection::prettyName() const
{
    return tr( "Your Collection" );
}
