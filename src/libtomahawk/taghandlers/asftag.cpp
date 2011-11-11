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

#include "asftag.h"

namespace Tomahawk
{

ASFTag::ASFTag( TagLib::Tag *tag, TagLib::ASF::Tag *asfTag )
    : Tag( tag )
    , m_asfTag( asfTag )
{
    TagLib::ASF::AttributeListMap map = m_asfTag->attributeListMap();
    for( TagLib::ASF::AttributeListMap::ConstIterator it = map.begin();
         it != map.end(); ++it )
    {
        TagLib::String key = it->first;
        QString val = TStringToQString( it->second[ 0 ].toString() );
        if( key == TagLib::String( "WM/AlbumTitle" ) ) //album artist
        {
            m_albumArtist = val;
        }
        else if( key == TagLib::String( "WM/Composer" ) )
        {
            m_composer = val;
        }
        else if( key == TagLib::String( "WM/PartOfSet" ) )
        {
            m_discNumber = processDiscNumber( val );
        }
    }
}

}
