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

#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"
#include "Query.h"
#include "SourceList.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "DropJobNotifier.h"
#include "ViewManager.h"

#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

using namespace Tomahawk;

QPixmap* SoundcloudParser::s_pixmap = 0;


SoundcloudParser::SoundcloudParser( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
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


SoundcloudParser::SoundcloudParser( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( true )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_type( DropJob::All )
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
    QUrl scLink( QString( "http://api.soundcloud.com/resolve.json?client_id=TiNg2DRYhBnp01DA3zNag&url=" ) + link );
    qDebug() << scLink.toString();
    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( scLink ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( soundcloudBrowseFinished() ) );

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

    if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
    {
        tLog() << "Didn't get an artist and track name from Soundcloud, not enough to build a query on. Aborting" << title << artist;
        return;
    }

    Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, QString(), uuid(), m_trackMode );

    if ( !q.isNull() )
    {
        tLog() << "Setting resulthint to " << res.value( "stream_url" ) << res;
        q->setResultHint( res.value( "stream_url" ).toString() + "?client_id=TiNg2DRYhBnp01DA3zNag" );
        q->setSaveHTTPResultHint( true );
        m_tracks << q;
    }

}

void
SoundcloudParser::soundcloudLookupFinished()
{

    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );
    m_queries.remove( r );
    r->deleteLater();

    if ( r->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( r, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse json from Soundcloud browse item :" << p.errorString() << "On line" << p.errorLine();
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
        else if ( m_type == DropJob::Artist )
        {
            // cant parse soundcloud json here atm.
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
SoundcloudParser::soundcloudBrowseFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    r->deleteLater();

    if ( r->error() == QNetworkReply::NoError )
    {
        if ( r->rawHeaderList().contains( "Location" ) )
        {
            QString url = r->rawHeader("Location");
            if ( url.contains( "tracks" ) )
            {
                m_type = DropJob::Track;
            }
            else if ( url.contains( "users" ) )
            {
                // For now, dont handle user tracklists
                m_type = DropJob::All; //DropJob::Artist;
                url = url.replace( ".json", "/tracks.json" );
                qDebug() << "Gots artist!" << url;
            }
            else if ( url.contains( "playlists" ) )
            {
                m_type = DropJob::Playlist;
            }

            if ( m_type != DropJob::All )
            {
                QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( QUrl(url) ) );
                connect( reply, SIGNAL( finished() ), this, SLOT( soundcloudLookupFinished() ) );
            }
        }
    }

    if ( m_type == DropJob::All )
    { // No good
        m_queries.remove( r );

        if ( m_browseJob )
            m_browseJob->setFinished();
        return;
    }

}

QPixmap
SoundcloudParser::pixmap() const
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/soundcloud.png" );

    return *s_pixmap;
}
