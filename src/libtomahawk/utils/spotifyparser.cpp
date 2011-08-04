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

#include "spotifyparser.h"

#include "utils/logger.h"
#include "utils/tomahawkutils.h"
#include "query.h"

#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

using namespace Tomahawk;

SpotifyParser::SpotifyParser( const QStringList& trackUrls, QObject* parent )
    : QObject ( parent )
    , m_single( false )
{
    foreach ( const QString& url, trackUrls )
        lookupTrack( url );
}

SpotifyParser::SpotifyParser( const QString& trackUrl, QObject* parent )
    : QObject ( parent )
    , m_single( true )
{
    lookupTrack( trackUrl );
}

SpotifyParser::~SpotifyParser()
{

}

void
SpotifyParser::lookupTrack( const QString& link )
{
    if ( !link.contains( "track" ) ) // we only support track links atm
        return;

    // we need Spotify URIs such as spotify:track:XXXXXX, so if we by chance get a http://open.spotify.com url, convert it
    QString uri = link;
    if ( link.contains( "open.spotify.com" ) )
    {
        QString hash = link;
        hash.replace( "http://open.spotify.com/track/", "" );
        uri = QString( "spotify:track:%1" ).arg( hash );
    }

    tLog() << "Parsing Spotify Track URI:" << uri;

    QUrl url = QUrl( QString( "http://ws.spotify.com/lookup/1/.json?uri=%1" ).arg( uri ) );
    tDebug() << "Looking up..." << url.toString();

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( spotifyTrackLookupFinished() ) );

    m_queries.insert( reply );

}

void
SpotifyParser::spotifyTrackLookupFinished()
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
            tLog() << "Failed to parse json from Spotify track lookup:" << p.errorString() << "On line" << p.errorLine();
            checkFinished();
            return;
        } else if ( !res.contains( "track" ) )
        {
            tLog() << "No 'track' item in the spotify track lookup result... not doing anything";
            checkFinished();
            return;
        }

        // lets parse this baby
        QVariantMap t = res.value( "track" ).toMap();

        QString title, artist, album;

        title = t.value( "name", QString() ).toString();
        // TODO for now only take the first artist
        if ( t.contains( "artists" ) && t[ "artists" ].canConvert< QVariantList >() && t[ "artists" ].toList().size() > 0 )
            artist = t[ "artists" ].toList().first().toMap().value( "name", QString() ).toString();
        if ( t.contains( "album" ) && t[ "album" ].canConvert< QVariantMap >() )
            album = t[ "album" ].toMap().value( "name", QString() ).toString();

        if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
        {
            tLog() << "Didn't get an artist and track name from spotify, not enough to build a query on. Aborting" << title << artist << album;
            return;
        }

        Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), true );
        m_tracks << q;

    } else
    {
        tLog() << "Error in network request to Spotify for track decoding:" << r->errorString();
    }

    checkFinished();
}

void
SpotifyParser::checkFinished()
{
    if ( m_queries.isEmpty() ) // we're done
    {
        if ( m_single && !m_tracks.isEmpty() )
            emit track( m_tracks.first() );
        else if ( !m_single && !m_tracks.isEmpty() )
            emit tracks( m_tracks );

        deleteLater();
    }

}
