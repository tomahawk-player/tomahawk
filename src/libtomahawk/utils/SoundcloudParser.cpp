/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
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

#include "SoundcloudParser.h"

#include <QtNetwork/QNetworkAccessManager>

#include <qjson/parser.h>

#include "Query.h"
#include "SourceList.h"
#include "DropJobNotifier.h"
#include "ViewManager.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


SoundcloudParser::SoundcloudParser( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_type( DropJob::All )
    , m_getLikes( false )

{
    foreach ( const QString& url, Urls )
        lookupUrl( url );
}


SoundcloudParser::SoundcloudParser( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( true )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_type( DropJob::All )
    , m_getLikes( false )
{
    lookupUrl( Url );
}


SoundcloudParser::~SoundcloudParser()
{
}


void
SoundcloudParser::lookupUrl( const QString& link )
{
    tDebug() << "Looking up URL..." << link;
    QString url = link;
    if ( link.contains( "/likes" ) )
    {
        qDebug() << Q_FUNC_INFO << "Requesting likes";
        url.replace( "/likes", "" );
        m_getLikes = true;
    }
    QUrl scLink( QString( "http://api.soundcloud.com/resolve.json?client_id=TiNg2DRYhBnp01DA3zNag&url=" ) + url );

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( scLink ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( soundcloudLookupFinished() ) );

    m_browseJob = new DropJobNotifier( pixmap(), "Soundcloud", DropJob::All, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );

    m_queries.insert( reply );
}


void
SoundcloudParser::parseTrack( const QVariantMap& res )
{
    QString title, artist;
    title = res.value( "title", QString() ).toString();
    artist = res.value( "user" ).toMap().value( "username", QString() ).toString();
    bool streamable = res.value( "streamable" ).toBool();

    if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
    {
        tLog() << "Didn't get an artist and track name from Soundcloud, not enough to build a query on. Aborting" << title << artist;
        return;
    }

    if ( !streamable )
    {
        JobStatusView::instance()->model()->addJob(
            new ErrorStatusMessage( tr( "Track '%1' by %2 is not streamable." ).arg( title ).arg( artist ), 5 ) );
        tLog() << "Track is not streamble, aborting." << res.value( "uri" ).toString();
        return;
    }

    Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, QString(), uuid(), m_trackMode );

    if ( !q.isNull() )
    {
        QUrl url = QUrl::fromUserInput( res.value( "stream_url" ).toString() );
        url.addQueryItem( "client_id", "TiNg2DRYhBnp01DA3zNag" );
        tLog() << "Setting resulthint to " << res.value( "stream_url" ) << url.toString();
        q->setResultHint( url.toString() );
        q->setSaveHTTPResultHint( true );
        m_tracks << q;
    }
}


void
SoundcloudParser::soundcloudLookupFinished()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );
    m_queries.remove( r );
    r->deleteLater();

    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( r->reply(), &ok ).toMap();
        if ( !ok )
        {
            tLog() << "Failed to parse json from Soundcloud browse item:" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        if( res.value( "kind" ).toString() == "playlist" )
            m_type = DropJob::Playlist;
        if( res.value( "kind" ).toString() == "track" )
            m_type = DropJob::Track;
        if( res.value( "kind" ).toString() == "user" )
        {
            QUrl url;
            if ( m_getLikes )
            {
                url = QUrl( QString( res.value( "uri" ).toString() + "/favorites.json?client_id=TiNg2DRYhBnp01DA3zNag" ) );
            }
            else
            {
                url = QUrl( QString( res.value( "uri" ).toString() + "/tracks.json?client_id=TiNg2DRYhBnp01DA3zNag" ) );
            }

            NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( QUrl( url ) ) ) );

            if ( m_createNewPlaylist )
                m_userData = res;

            connect( reply, SIGNAL( finished() ), SLOT( soundcloudArtistLookupFinished() ) );
            return;
        }

        if ( m_type == DropJob::Track )
        {
            parseTrack( res );
        }
        else if ( m_type == DropJob::Playlist )
        {
            QString title, user, desc;
            title = res.value( "title" ).toString();
            desc = res.value( "description" ).toString();
            user = res.value( "user" ).toMap().value( "username" ).toString();

            QVariantList tracks = res.value( "tracks" ).toList();

            foreach( const QVariant& track, tracks )
                parseTrack( track.toMap() );

            if ( m_createNewPlaylist )
            {
                m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                           uuid(),
                                           title,
                                           desc,
                                           user,
                                           false,
                                           m_tracks );

                connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );
                return;
            }
        }
    }

    if ( m_single && !m_tracks.isEmpty() )
        emit track( m_tracks.first() );
    else if ( !m_single && !m_tracks.isEmpty() )
        emit tracks( m_tracks );

    deleteLater();
}


void
SoundcloudParser::playlistCreated()
{
    ViewManager::instance()->show( m_playlist );

    deleteLater();
}


void
SoundcloudParser::soundcloudArtistLookupFinished()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );

    r->deleteLater();

    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantList res = p.parse( r->reply(), &ok ).toList();

        foreach( const QVariant& track, res )
            parseTrack( track.toMap() );

        if ( m_createNewPlaylist )
        {
            const QString user = m_userData.value( "full_name" ).toString();
            const QString title = user + "'s " + ( m_getLikes ? "Favorites" : "Tracks" );
            m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       title,
                                       "",
                                       user,
                                       false,
                                       m_tracks );

            connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );
            return;

        }
        if ( m_single && !m_tracks.isEmpty() )
            emit track( m_tracks.first() );
        else if ( !m_single && !m_tracks.isEmpty() )
            emit tracks( m_tracks );

        deleteLater();

    }
}


QPixmap
SoundcloudParser::pixmap() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::SoundcloudIcon );
}
