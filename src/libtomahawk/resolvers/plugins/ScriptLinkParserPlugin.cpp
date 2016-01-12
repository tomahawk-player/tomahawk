/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2016,    Dominik Schmidt <domme@tomahawk-player.org>
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
#include "ScriptLinkParserPlugin_p.h"

#include "../ScriptJob.h"
#include "../ScriptObject.h"
#include "../../utils/Logger.h"
#include "../ScriptAccount.h"

#include "../../database/Database.h"
#include "../../database/DatabaseImpl.h"
#include "../../SourceList.h"
#include "../../Artist.h"
#include "../../Album.h"
#include "../../playlist/PlaylistTemplate.h"
#include "../../playlist/XspfPlaylistTemplate.h"


using namespace Tomahawk;

ScriptLinkParserPlugin::ScriptLinkParserPlugin( const scriptobject_ptr& scriptObject, ScriptAccount* account )
    : Utils::LinkParserPlugin()
    , ScriptPlugin( scriptObject )
    , d_ptr( new ScriptLinkParserPluginPrivate( this, scriptObject, account ) )
{
}


ScriptLinkParserPlugin::~ScriptLinkParserPlugin()
{
}


bool
ScriptLinkParserPlugin::canParseUrl( const QString& url, Tomahawk::Utils::UrlType type ) const
{
    QVariantMap arguments;
    arguments["url"] = url;
    arguments["type"] = (int) type;

    return scriptObject()->syncInvoke( "canParseUrl", arguments ).toBool();
}


void
ScriptLinkParserPlugin::lookupUrl( const QString& url ) const
{
    QVariantMap arguments;
    arguments["url"] = url;
    Tomahawk::ScriptJob* job = scriptObject()->invoke( "lookupUrl", arguments );
    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( onLookupUrlRequestDone( QVariantMap ) ) );
    job->setProperty( "url", url );
    job->start();
}


void
ScriptLinkParserPlugin::onLookupUrlRequestDone( const QVariantMap& result )
{
    Q_D( ScriptLinkParserPlugin );

    sender()->deleteLater();

    QString url = sender()->property( "url" ).toString();

    tLog() << "ON LOOKUP URL REQUEST DONE" << url << result;

    // It may seem a bit weird, but currently no slot should do anything
    // more as we starting on a new URL and not task are waiting for it yet.
    d->pendingUrl = QString();
    d->pendingAlbum = album_ptr();

    Utils::UrlType type = (Utils::UrlType) result.value( "type" ).toInt();
    if ( type == Utils::UrlTypeArtist )
    {
        QString name = result.value( "name" ).toString();
        Q_ASSERT( !name.isEmpty() );
        emit informationFound( url, Artist::get( name, true ).objectCast<QObject>() );
    }
    else if ( type == Utils::UrlTypeAlbum )
    {
        QString name = result.value( "name" ).toString();
        QString artist = result.value( "artist" ).toString();
        album_ptr album = Album::get( Artist::get( artist, true ), name );
        d->pendingUrl = url;
        d->pendingAlbum = album;
        connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                 SLOT( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ) );
        if ( !album->tracks().isEmpty() )
        {
            emit informationFound( url, album.objectCast<QObject>() );
        }
    }
    else if ( type == Utils::UrlTypeTrack )
    {
        Tomahawk::query_ptr query = parseTrack( result );
        if ( query.isNull() )
        {
            // A valid track result shoud have non-empty title and artist.
            tLog() << Q_FUNC_INFO << d->scriptAccount->name() << "Got empty track information for " << url;
            emit informationFound( url, QSharedPointer<QObject>() );
        }
        else
        {
            emit informationFound( url, query.objectCast<QObject>() );
        }
    }
    else if ( type == Utils::UrlTypePlaylist )
    {
        QString guid = result.value( "guid" ).toString();
        Q_ASSERT( !guid.isEmpty() );
        // Append nodeid to guid to make it globally unique.
        guid += instanceUUID();

        // Do we already have this playlist loaded?
        {
            playlist_ptr playlist = Playlist::get( guid );
            if ( !playlist.isNull() )
            {
                emit informationFound( url, playlist.objectCast<QObject>() );
                return;
            }
        }

        // Get all information to build a new playlist but do not build it until we know,
        // if it is really handled as a playlist and not as a set of tracks.
        Tomahawk::source_ptr source = SourceList::instance()->getLocal();
        const QString title = result.value( "title" ).toString();
        const QString info = result.value( "info" ).toString();
        const QString creator = result.value( "creator" ).toString();
        QList<query_ptr> queries;
        foreach( QVariant track, result.value( "tracks" ).toList() )
        {
            query_ptr query = parseTrack( track.toMap() );
            if ( !query.isNull() )
            {
                queries << query;
            }
        }
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << d->scriptAccount->name() << "Got playlist for " << url;
        playlisttemplate_ptr pltemplate( new PlaylistTemplate( source, guid, title, info, creator, false, queries ) );
        emit informationFound( url, pltemplate.objectCast<QObject>() );
    }
    else if ( type == Utils::UrlTypeXspf )
    {
        QString xspfUrl = result.value( "url" ).toString();
        Q_ASSERT( !xspfUrl.isEmpty() );
        QString guid = QString( "xspf-%1-%2" ).arg( xspfUrl.toUtf8().toBase64().constData() ).arg( instanceUUID() );

        // Do we already have this playlist loaded?
        {
            playlist_ptr playlist = Playlist::get( guid );
            if ( !playlist.isNull() )
            {
                emit informationFound( url, playlist.objectCast<QObject>() );
                return;
            }
        }


        // Get all information to build a new playlist but do not build it until we know,
        // if it is really handled as a playlist and not as a set of tracks.
        Tomahawk::source_ptr source = SourceList::instance()->getLocal();
        QSharedPointer<XspfPlaylistTemplate> pltemplate( new XspfPlaylistTemplate( xspfUrl, source, guid ) );
        NewClosure( pltemplate, SIGNAL( tracksLoaded( QList< Tomahawk::query_ptr > ) ),
                    this, SLOT( pltemplateTracksLoadedForUrl( QString, Tomahawk::playlisttemplate_ptr ) ),
                    url, pltemplate.objectCast<Tomahawk::PlaylistTemplate>() );
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << d->scriptAccount->name() << "Got playlist for " << url;
        pltemplate->load();
    }
    else
    {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << d->scriptAccount->name() << "No usable information found for " << url;
        emit informationFound( url, QSharedPointer<QObject>() );
    }
}

void
ScriptLinkParserPlugin::pltemplateTracksLoadedForUrl( const QString& url, const playlisttemplate_ptr& pltemplate )
{
    tLog() << Q_FUNC_INFO;
    emit informationFound( url, pltemplate.objectCast<QObject>() );
}


QString
ScriptLinkParserPlugin::instanceUUID()
{
    return Tomahawk::Database::instance()->impl()->dbid();
}


Tomahawk::query_ptr
ScriptLinkParserPlugin::parseTrack( const QVariantMap& track )
{
    QString title = track.value( "track" ).toString();
    QString artist = track.value( "artist" ).toString();
    QString album = track.value( "album" ).toString();
    if ( title.isEmpty() || artist.isEmpty() )
    {
        return query_ptr();
    }

    Tomahawk::query_ptr query = Tomahawk::Query::get( artist, title, album );
    QString resultHint = track.value( "hint" ).toString();
    if ( !resultHint.isEmpty() )
    {
        query->setResultHint( resultHint );
        query->setSaveHTTPResultHint( true );
    }

    return query;
}
