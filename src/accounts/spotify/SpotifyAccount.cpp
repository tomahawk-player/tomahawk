/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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
#include "playlist/PlaylistUpdaterInterface.h"
#include "sourcelist.h"
#include "SpotifyAccountConfig.h"
#include "resolvers/scriptresolver.h"
#include <QPixmap>

using namespace Tomahawk;
using namespace Accounts;

static QPixmap* s_icon = 0;

Account*
SpotifyAccountFactory::createAccount( const QString& accountId )
{
    return new SpotifyAccount( accountId );
}


bool
SpotifyAccountFactory::acceptsPath( const QString& path ) const
{
    QFileInfo info( path );
    return info.baseName().startsWith( "spotify_" );
}


Account*
SpotifyAccountFactory::createFromPath( const QString& path )
{
    return new SpotifyAccount( generateId( factoryId() ), path );
}


QPixmap
SpotifyAccountFactory::icon() const
{
    if ( !s_icon )
        s_icon = new QPixmap( RESPATH "images/spotify-logo.png" );

    return *s_icon;
}


SpotifyAccount::SpotifyAccount( const QString& accountId )
    : ResolverAccount( accountId )
{
    init();
}


SpotifyAccount::SpotifyAccount( const QString& accountId, const QString& path )
    : ResolverAccount( accountId, path )
{
    init();
}


void
SpotifyAccount::init()
{
    m_spotifyResolver = dynamic_cast< ScriptResolver* >( m_resolver.data() );

    connect( m_spotifyResolver.data(), SIGNAL( customMessage( QString,QVariantMap ) ), this, SLOT( resolverMessage( QString, QVariantMap ) ) );

    const bool hasMigrated = configuration().value( "hasMigrated" ).toBool();
    if ( !hasMigrated )
    {
        qDebug() << "Getting credentials from spotify resolver to migrate to in-app config";
        QVariantMap msg;
        msg[ "_msgtype" ] = "getCredentials";
        m_spotifyResolver.data()->sendMessage( msg );
    }
}


void
SpotifyAccount::resolverMessage( const QString &msgType, const QVariantMap &msg )
{
    if ( msgType == "credentials" )
    {
        QVariantHash creds = credentials();
        creds[ "username" ] = msg.value( "username" );
        creds[ "password" ] = msg.value( "password" );
        creds[ "highQuality" ] = msg.value( "highQuality" );
        setCredentials( creds );
        sync();

        QVariantHash config = configuration();
        config[ "hasMigrated" ] = true;
        setConfiguration( config );
        sync();
    }
}


QPixmap
SpotifyAccount::icon() const
{
    if ( !s_icon )
        s_icon = new QPixmap( RESPATH "images/spotify-logo.png" );

    return *s_icon;
}


QWidget*
SpotifyAccount::configurationWidget()
{
    if ( m_configWidget.isNull() )
        m_configWidget = QWeakPointer< SpotifyAccountConfig >( new SpotifyAccountConfig( this ) );
    else
        m_configWidget.data()->loadFromConfig();

    return static_cast< QWidget* >( m_configWidget.data() );
}


void
SpotifyAccount::saveConfig()
{
    Q_ASSERT( !m_configWidget.isNull() );
    if ( m_configWidget.isNull() )
        return;

    // Send the result to the resolver
    QVariantMap msg;
    msg[ "_msgtype" ] = "saveSettings";
    msg[ "username" ] = m_configWidget.data()->username();
    msg[ "password" ] = m_configWidget.data()->password();
    msg[ "highQuality" ] = m_configWidget.data()->highQuality();

    m_spotifyResolver.data()->sendMessage( msg );
}


void
SpotifyAccount::addPlaylist( const QString &qid, const QString& title, QList< Tomahawk::query_ptr > tracks )
{
/*    Sync sync;
    sync.id_ = qid;
    int index =  m_syncPlaylists.indexOf( sync );

    if(  !m_syncPlaylists.contains( sync ) )
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
    else
    {

        qDebug() << Q_FUNC_INFO << "Found playlist";

        if ( index != -1 && !tracks.isEmpty())
        {

            qDebug() << Q_FUNC_INFO << "Got pl" << m_syncPlaylists[ index ].playlist->guid();

            QList< query_ptr > currTracks;
            foreach ( const plentry_ptr ple, m_syncPlaylists[ index ].playlist->entries() )
                currTracks << ple->query();

            qDebug() << Q_FUNC_INFO << "tracks" << currTracks;

            bool changed = false;
            QList< query_ptr > mergedTracks = TomahawkUtils::mergePlaylistChanges( currTracks, tracks, changed );

            if ( changed )
            {
                QList<Tomahawk::plentry_ptr> el = m_syncPlaylists[ index ].playlist->entriesFromQueries( mergedTracks, true );
                m_syncPlaylists[ index ].playlist->createNewRevision( uuid(), m_syncPlaylists[ index ].playlist->currentrevision(), el );
            }
        }
    }

    */
}


bool
operator==( Accounts::SpotifyAccount::Sync one, Accounts::SpotifyAccount::Sync two )
{
    if( one.id_ == two.id_ )
        return true;
    return false;
}
