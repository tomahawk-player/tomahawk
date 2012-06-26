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

#ifndef ID3V2TAG_H
#define ID3V2TAG_H

#include "tag.h"
#include <taglib/id3v2tag.h>

namespace Tomahawk
{

class DLLEXPORT ID3v2Tag : public Tag
{
public:
    ID3v2Tag( TagLib::Tag *, TagLib::ID3v2::Tag * );

    virtual QString albumArtist() const { return m_albumArtist; }
    virtual QString composer() const { return m_composer; }
    virtual unsigned int discNumber() const { return m_discNumber; }

private:
    TagLib::ID3v2::Tag *m_id3v2Tag;
};

}

#endif // ID3V2TAG_H
