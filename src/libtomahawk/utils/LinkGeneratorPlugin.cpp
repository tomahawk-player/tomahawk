/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2014  Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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
#include "LinkGeneratorPlugin.h"

#include "../Query.h"
#include "../Track.h"
#include "../Artist.h"
#include "../Album.h"

Tomahawk::Utils::LinkGeneratorPlugin::~LinkGeneratorPlugin()
{
}


Tomahawk::ScriptJob*
Tomahawk::Utils::LinkGeneratorPlugin::openLink(const QString&, const QString&, const QString&) const
{
    return nullptr;
}


Tomahawk::ScriptJob*
Tomahawk::Utils::LinkGeneratorPlugin::openLink( const Tomahawk::query_ptr& query ) const
{
    QString title = query->track()->track();
    QString artist = query->track()->artist();
    QString album = query->track()->album();

    return openLink( title, artist, album );
}


Tomahawk::ScriptJob*
Tomahawk::Utils::LinkGeneratorPlugin::openLink(const Tomahawk::artist_ptr&) const
{
    return nullptr;
}


Tomahawk::ScriptJob*
Tomahawk::Utils::LinkGeneratorPlugin::openLink(const Tomahawk::album_ptr&) const
{
    return nullptr;
}


Tomahawk::ScriptJob*
Tomahawk::Utils::LinkGeneratorPlugin::openLink(const Tomahawk::dynplaylist_ptr&) const
{
    return nullptr;
}
