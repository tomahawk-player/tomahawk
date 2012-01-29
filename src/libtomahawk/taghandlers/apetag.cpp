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

#include "apetag.h"

namespace Tomahawk
{

APETag::APETag( TagLib::Tag *tag, TagLib::APE::Tag *apeTag )
    : Tag( tag )
    , m_apeTag( apeTag )
{
    TagLib::APE::ItemListMap map = m_apeTag->itemListMap();
    for( TagLib::APE::ItemListMap::ConstIterator it = map.begin();
         it != map.end(); ++it )
    {
        TagLib::String key = it->first;
        QString val = TStringToQString( it->second.toString() );
        //some of these are not defined in the item key according to the hydrogenaudio wiki
        //can I use them anyway?    --Teo 11/2011
        if( key == TagLib::String( "Album Artist" ) )
        {
            m_albumArtist = val;
        }
        else if( key == TagLib::String( "Composer" ) )
        {
            m_composer = val;
        }
        else if( key == TagLib::String( "Disc" ) )
        {
            m_discNumber = processDiscNumber( val );
        }
    }
}

}
