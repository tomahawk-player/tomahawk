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

#include "id3v2tag.h"
#include <taglib/textidentificationframe.h>

namespace Tomahawk
{

ID3v2Tag::ID3v2Tag( TagLib::Tag *tag, TagLib::ID3v2::Tag *id3v2Tag )
    : Tag( tag )
    , m_id3v2Tag( id3v2Tag )
{
    TagLib::ID3v2::FrameList fList = m_id3v2Tag->frameList();
    for( TagLib::ID3v2::FrameList::ConstIterator it = fList.begin();
         it != fList.end(); ++it )
    {
        TagLib::String frameId = TagLib::String( (*it)->frameID() );
        TagLib::ID3v2::TextIdentificationFrame *frame =
                dynamic_cast< TagLib::ID3v2::TextIdentificationFrame * >( *it );
        if( frame )
        {
            QString val = TStringToQString( frame->fieldList().toString( '\n' ) );

            if( frameId == TagLib::String( "TPE2" ) ) //album artist
            {
                m_albumArtist = val;
            }
            else if( frameId == TagLib::String( "TCOM" ) ) //composer
            {
                m_composer = val;
            }
            else if( frameId == TagLib::String( "TPOS" ) ) //disc number
            {
                m_discNumber = processDiscNumber( val );
            }
        }
    }
}

}
