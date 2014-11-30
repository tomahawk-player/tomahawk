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

#pragma once
#ifndef TOMAHAWK_UTILS_TOMAHKLINKGENERATORPLUGIN_H
#define TOMAHAWK_UTILS_TOMAHKLINKGENERATORPLUGIN_H

#include "LinkGeneratorPlugin.h"

#include "../DllMacro.h"
#include "../Typedefs.h"

#include <echonest/Playlist.h>

namespace Tomahawk {

class ScriptJob;

namespace Utils {

class DLLEXPORT TomaHkLinkGeneratorPlugin : public LinkGeneratorPlugin
{
public:
    ScriptJob* openLink( const QString& title, const QString& artist, const QString& album ) const override;
    ScriptJob* openLink( const Tomahawk::artist_ptr& artist ) const override;
    ScriptJob* openLink( const Tomahawk::album_ptr& album ) const override;
    ScriptJob* openLink( const Tomahawk::dynplaylist_ptr& playlist ) const override;

private:
    QString hostname() const;

};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_TOMAHKLINKGENERATORPLUGIN_H
