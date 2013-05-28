/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Hugo Lindström <hugolm84@gmail.com>
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

#include "ItunesLoader.h"

#include "Playlist.h"
#include "SourceList.h"
#include "utils/Uuid.h"

#include <QUrl>
#include <QSettings>

using namespace Tomahawk;

ItunesLoader::ItunesLoader( const QString& input, QObject *parent )
    : QObject(parent)
    , m_itunesLibFile( input )
{
    /**
     * Paths to ItunesLibFile
     *  /Users/username/Music/iTunes/iTunes Library.xml
     *  \Users\username\Music\iTunes\iTunes Library.xml
     *  \Documents and Settings\username\My Documents\My Music\iTunes\iTunes Library.xml
     *  http://support.apple.com/kb/HT1660
     */

    QSettings plist( QUrl::fromUserInput( QString( input.simplified() ) ).toLocalFile(), QSettings::NativeFormat);

    m_ignoreFields << "Library" << "Movies" << "TV Shows" << "Music Videos" << "Genius";

    parseTracks( plist.value( "Tracks" ).toMap() );
    parsePlaylists( plist.value( "Playlists" ).toList() );

    foreach ( const QString& name, m_playlists.keys())
    {
        Playlist::create( SourceList::instance()->getLocal(),
                          uuid(),
                          name,
                          "iTunes imported playlist",
                          "",
                          false,
                          m_playlists[ name ] );
    }

    m_tracks.clear();
    m_playlists.clear();
}


void
ItunesLoader::parseTracks( const QVariantMap& tracks )
{
    foreach ( const QVariant& track, tracks )
    {
        QVariantMap trackMap = track.toMap();
        if ( !trackMap.value( "Track ID" ).isValid() )
            continue;

        const QString artist = trackMap.value( "Artist", "" ).toString();
        const QString title = trackMap.value( "Name", "" ).toString();
        const QString album = trackMap.value( "Album", "" ).toString();

        if ( artist.isEmpty() || title.isEmpty() )
            continue;

        m_tracks.insert( trackMap.value( "Track ID" ).toInt(), Tomahawk::Query::get( artist, title, album ) );
    }
}


void
ItunesLoader::parsePlaylists( const QVariantList& playlists )
{
    foreach ( const QVariant& playlist, playlists )
    {
        const QString title = playlist.toMap().value( "Name" ).toString();

        if ( m_ignoreFields.contains( title ) )
            continue;

        foreach ( const QVariant& items, playlist.toMap() )
        {
            if ( items.toMap().value( "Track ID" ).isValid() )
            {
                int trackId = items.toMap().value( "Track ID" ).toInt();
                if ( m_tracks.contains( trackId ) )
                    m_playlists[title] << m_tracks[ trackId ];
            }
        }
    }
}
