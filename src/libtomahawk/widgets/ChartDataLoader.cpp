/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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
#include "ChartDataLoader.h"

#include "Source.h"

using namespace Tomahawk;


ChartDataLoader::ChartDataLoader()
    : QObject( 0 )
{
}


void
ChartDataLoader::go()
{
    switch ( m_type )
    {
        case Track:
        {
            QList< query_ptr > track_ptrs;
            foreach ( const Tomahawk::InfoSystem::InfoStringHash& track, m_data )
            {
                query_ptr q = Query::get( track[ "artist" ], track[ "track" ], QString(), uuid(), false );
                if ( q.isNull() )
                    continue;

                const QString url = track[ "streamUrl" ];
                if ( !url.isEmpty() )
                {
                    q->setResultHint( url );
                    q->setSaveHTTPResultHint( true );
                }

                track_ptrs << q;
            }

            emit tracks( this, track_ptrs );
            break;
        }
        case Artist:
        {
            QList< artist_ptr > artist_ptrs;

            foreach ( const QString& artistname, m_artists )
            {
                artist_ptrs << Artist::get( artistname, false );
            }

            emit artists( this, artist_ptrs );
            break;
        }
        case Album:
        {
            QList< album_ptr > album_ptrs;

            foreach ( const Tomahawk::InfoSystem::InfoStringHash& album, m_data )
            {
                if ( album["artist"].isEmpty() )
                    continue;

                artist_ptr artistPtr = Artist::get( album[ "artist" ], false );
                album_ptr albumPtr = Album::get( artistPtr, album[ "album" ], false );
                album_ptrs << albumPtr;
            }

            emit albums( this, album_ptrs );
            break;
        }
    }
}
