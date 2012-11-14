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

#include "ExfmParser.h"

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
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;

QPixmap* ExfmParser::s_pixmap = 0;


ExfmParser::ExfmParser( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_type( DropJob::All )

{
    foreach ( const QString& url, Urls )
        lookupUrl( url );
}


ExfmParser::ExfmParser( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( true )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_type( DropJob::All )
{
    lookupUrl( Url );
}


ExfmParser::~ExfmParser()
{
}


void
ExfmParser::lookupUrl( const QString& link )
{
    const QString apiBase = "http://ex.fm/api/v3";
    QString url( link );
    QStringList paths;
    foreach( const QString& path, QUrl( link ).path().split( "/" ) )
    {
        if ( !path.isEmpty() )
            paths << path;
    }

    // User request
    if ( paths.count() == 1 )
    {
        m_type = DropJob::Artist;
        url = QString( apiBase + "/user/%1/trending" ).arg( paths.takeFirst() );
    }
    else
    {
        if ( url.contains( "/explore/site-of-the-day" ) ) // We treat site as artist
        {
            m_type = DropJob::Artist;
            url.replace( "/explore/site-of-the-day", "/sotd?results=1" );
        }
        else if ( url.contains( "/song/" ) )
        {
            m_type = DropJob::Track;
        }
        else if( ( url.contains( "site") && url.contains( "album" ) ) || url.contains( "mixtape-of-the-month" ) )
        {
            m_type = DropJob::Album;
            if ( url.contains( "album-of-the-week") )
                url = QString( apiBase + "/%1?%2" ).arg( "aotw" ).arg( "results=1" ); // Only need the first album, eg. this week
            if ( url.contains( "mixtape-of-the-month" ) )
                url = QString( apiBase + "/%1?%2" ).arg( "motm" ).arg( "results=1" ); // Only need the first mixtape, eg. this week
        }
        else
        {
            m_type = DropJob::Playlist;
            if ( url.contains( "tastemakers" ) )
                url.replace( "trending", "explore" );
        }
        if ( m_type == DropJob::Playlist )
        {
            url.replace( "/genre/", "/tag/" );
            url.replace( "/search/", "/song/search/" ); // We can only search for tracks, even though we want an artist or whatever
        }

        url.replace( "http://ex.fm", apiBase );
    }

    tDebug() << "Looking up URL..." << url;
    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( QUrl( url ) ) ) );

    if ( m_createNewPlaylist )
        connect( reply, SIGNAL( finished() ), SLOT( exfmLookupFinished() ) );
    else
        connect( reply, SIGNAL( finished() ), SLOT( exfmBrowseFinished() ) );

    m_browseJob = new DropJobNotifier( pixmap(), "Exfm", m_type, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );

    m_queries.insert( reply );
}


void
ExfmParser::parseTrack( const QVariantMap& res )
{
    QString title, artist, album;
    album = res.value( "album", QString() ).toString();
    title = res.value( "title", QString() ).toString();
    artist = res.value( "artist", QString() ).toString();

    if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
    {
        tLog() << "Didn't get an artist and track name from Exfm, not enough to build a query on. Aborting" << title << artist;
        return;
    }

    Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), m_trackMode );

    if ( !q.isNull() )
    {
        tLog() << "Setting resulthint to " << res.value( "url" );
        q->setResultHint( res.value( "url" ).toString() );
        q->setProperty( "annotation", res.value( "url" ).toString() );
        m_tracks << q;
    }
}


void
ExfmParser::exfmLookupFinished()
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
            tLog() << "Failed to parse json from Exfm browse item:" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        QStringList paths;
        foreach ( const QString& path, r->reply()->url().path().split( "/" ) )
        {
            if ( !path.isEmpty() )
                paths << path;
        }

        QString title, artist, desc;
        QVariantList tracks;

        if ( m_type == DropJob::Album )
        {
            QStringList meta = res.value( "site" ).toMap().value( "title" ).toString().split( ", by " );
            title = meta.takeFirst();
            artist = meta.takeLast();
            tracks = res.value( "site" ).toMap().value( "songs" ).toList();
        }
        else
        {
            // Take endpoint as title
            title = paths.takeLast();
            title[0] = title[0].toUpper();

            if ( paths.contains( "trending") )
                title = "Trending " + title;
            else if ( paths.contains( "explore" ) )
                title = "Explore " + title;

            if ( paths.contains( "user" ) )
            {
                int index = paths.indexOf( "user");
                if ( index != -1 && paths.size() > index+1 )
                    artist = paths.takeAt(paths.indexOf( "user" ) +1 );
            }else
                artist = "Ex.fm";

            tracks = res.value( "songs" ).toList();
        }

        // Make the title
        title = title + " by " + artist;

        foreach( const QVariant& track, tracks )
            parseTrack( track.toMap() );

        m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       title,
                                       desc,
                                       artist,
                                       false,
                                       m_tracks );

        connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );
        return;
    }
}


void
ExfmParser::playlistCreated()
{
    ViewManager::instance()->show( m_playlist );

    deleteLater();
}


void
ExfmParser::exfmBrowseFinished()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );

    r->deleteLater();

    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( r->reply(), &ok ).toMap();

        if ( !ok )
        {
            qDebug() << "Failed to parse ex.fm json";
            return;
        }

        if ( m_type != DropJob::Track )
        {
            QVariantList tracks;
            if( m_type == DropJob::Album )
                tracks = res.value( "site" ).toMap().value( "songs" ).toList();
            else
                tracks = res.value( "songs" ).toList();

            foreach( const QVariant& track, tracks )
                parseTrack( track.toMap() );
        }
        else
            parseTrack( res.value( "song" ).toMap() );

        if ( m_single && !m_tracks.isEmpty() )
            emit track( m_tracks.first() );
        else if ( !m_single && !m_tracks.isEmpty() )
            emit tracks( m_tracks );

        deleteLater();
    }
}


QPixmap
ExfmParser::pixmap() const
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/exfm.png" );

    return *s_pixmap;
}
