/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2016  Dominik Schmidt <domme@tomahawk-player.org>
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
#ifndef TOMAHAWK_UTILS_URLTYPE_H
#define TOMAHAWK_UTILS_URLTYPE_H

namespace Tomahawk {

namespace Utils {

enum UrlType
{
    UrlTypeAny =       0x00,
    UrlTypePlaylist =  0x01,
    UrlTypeTrack =     0x02,
    UrlTypeAlbum =     0x04,
    UrlTypeArtist =    0x08,
    UrlTypeXspf =      0x10
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_URLTYPE_H
