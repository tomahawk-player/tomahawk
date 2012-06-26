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

#include "oggtag.h"

namespace Tomahawk
{

OggTag::OggTag( TagLib::Tag *tag, TagLib::Ogg::XiphComment *xiphComment )
    : Tag( tag )
    , m_xiphComment( xiphComment )
{
    TagLib::Ogg::FieldListMap map = m_xiphComment->fieldListMap();
    for( TagLib::Ogg::FieldListMap::ConstIterator it = map.begin();
         it != map.end(); ++it )
    {
        TagLib::String key = it->first;
        QString val = TStringToQString( it->second.toString( '\n' ) );
        if( key == TagLib::String( "ALBUMARTIST" ) )
        {
            m_albumArtist = val;
        }
        else if( key == TagLib::String( "COMPOSER" ) )
        {
            m_composer = val;
        }
        else if( key == TagLib::String( "DISCNUMBER" ) )
        {
            m_discNumber = processDiscNumber( val );
        }
    }
}

}
