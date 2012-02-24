/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "SpotifyAccount.h"
#include "playlist.h"
#include "utils/tomahawkutils.h"
#include "PlaylistUpdaterInterface.h"
#include "sourcelist.h"

using namespace Tomahawk;
using namespace Accounts;

SpotifyResolverAccount::SpotifyResolverAccount()
{
    qDebug() << Q_FUNC_INFO;
}
SpotifyResolverAccount::~SpotifyResolverAccount()
{
    qDebug() << Q_FUNC_INFO;
}

void
SpotifyResolverAccount::addPlaylist(const QString &qid, const QString& title, QList< Tomahawk::query_ptr > tracks)
{
    Sync sync;
    sync.id_ = qid;
    int index =  m_syncPlaylists.indexOf( sync );

    if( !m_syncPlaylists.contains( sync ) )
    {
         qDebug() << Q_FUNC_INFO << "Adding playlist to sync" << qid;
         playlist_ptr pl;
         pl = Tomahawk::Playlist::create( SourceList::instance()->getLocal(),
                                                   uuid(),
                                                   title,
                                                   QString(),
                                                   QString(),
                                                   false,
                                                   tracks );
         sync.playlist = pl;
         sync.uuid = pl->guid();
         m_syncPlaylists.append( sync );
    }
    else{

        qDebug() << Q_FUNC_INFO << "Found playlist";

        if( index != -1 && !tracks.isEmpty())
        {

            qDebug() << Q_FUNC_INFO << "Got pl" << m_syncPlaylists[ index ].playlist->guid();

            QList< query_ptr > currTracks;
            foreach ( const plentry_ptr ple, m_syncPlaylists[ index ].playlist->entries() )
                currTracks << ple->query();

            qDebug() << Q_FUNC_INFO << "tracks" << currTracks;

            QList< query_ptr > mergedTracks = TomahawkUtils::mergePlaylistChanges( currTracks, tracks );

            QList<Tomahawk::plentry_ptr> el = m_syncPlaylists[ index ].playlist->entriesFromQueries( mergedTracks, true );
            m_syncPlaylists[ index ].playlist->createNewRevision( uuid(), m_syncPlaylists[ index ].playlist->currentrevision(), el );

        }
    }


}



bool operator==(SpotifyResolverAccount::Sync one, SpotifyResolverAccount::Sync two)
{
    if(one.id_ == two.id_)
        return true;
    return false;
}

