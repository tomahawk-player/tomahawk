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

#include "mp4tag.h"

namespace Tomahawk
{

MP4Tag::MP4Tag( TagLib::Tag *tag, TagLib::MP4::Tag *mp4Tag )
    : Tag( tag )
    , m_mp4Tag( mp4Tag )
{
    TagLib::MP4::ItemListMap map = m_mp4Tag->itemListMap();
    for( TagLib::MP4::ItemListMap::ConstIterator it = map.begin();
         it != map.end(); ++it )
    {
        TagLib::String key = it->first;
        QString val = TStringToQString( it->second.toStringList().toString( '\n' ) );
        if( key == TagLib::String( "aART" ) )   //album artist
        {
            m_albumArtist = val;
        }
        else if( key == TagLib::String( "\xA9wrt" ) ) //composer
        {
            m_composer = val;
        }
        else if( key == TagLib::String( "disk" ) ) //disk number
        {
            m_discNumber = processDiscNumber( val );
        }
    }
}

}
