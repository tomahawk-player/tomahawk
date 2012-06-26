/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Teo Mrnjavac <teo@kde.org>
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

#ifndef ID3V1TAG_H
#define ID3V1TAG_H

#include "tag.h"
#include <taglib/id3v1tag.h>

namespace Tomahawk
{

class DLLEXPORT ID3v1Tag : public Tag
{
public:
    ID3v1Tag( TagLib::Tag * );

    virtual QString albumArtist() const { return QString(); }
    virtual QString composer() const { return QString(); }
    virtual unsigned int discNumber() const { return 0; }
};

}

#endif // ID3V1TAG_H
