/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "LocalCollection.h"

#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"

#include "PlaylistEntry.h"
#include "SourceList.h"
#include <TomahawkSettings.h>

#ifndef ENABLE_HEADLESS
    #include "ViewManager.h"
#endif

using namespace Tomahawk;

LocalCollection::LocalCollection( const Tomahawk::source_ptr& source, QObject* parent )
    : DatabaseCollection( source, parent )
{

}


QString
LocalCollection::prettyName() const
{
    return tr( "My Collection" );
}


QString
LocalCollection::emptyText() const
{
    return tr( "After you have scanned your music collection you will find your tracks right here." );
}
