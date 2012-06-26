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

#ifndef OGGTAG_H
#define OGGTAG_H

#include "tag.h"
#include <taglib/xiphcomment.h>

namespace Tomahawk
{

class DLLEXPORT OggTag : public Tag
{
public:
    OggTag( TagLib::Tag *, TagLib::Ogg::XiphComment * );

    virtual QString albumArtist() const { return m_albumArtist; }
    virtual QString composer() const { return m_composer; }
    virtual unsigned int discNumber() const { return m_discNumber; }

private:
    TagLib::Ogg::XiphComment *m_xiphComment;
};

}

#endif // OGGTAG_H
