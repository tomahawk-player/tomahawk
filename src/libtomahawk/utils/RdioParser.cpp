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

#include "RdioParser.h"

#include "ShortenedLinkParser.h"
#include "config.h"
#include "DropJob.h"
#include "DropJobNotifier.h"
#include "ViewManager.h"
#include "SourceList.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


#include <qjson/parser.h>

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QStringList>
#include <QCryptographicHash>

using namespace Tomahawk;

QPixmap* RdioParser::s_pixmap = 0;

#ifdef QCA2_FOUND
QCA::Initializer RdioParser::m_qcaInit = QCA::Initializer();
#endif


RdioParser::RdioParser( QObject* parent )
    : QObject( parent )
    , m_count( 0 )
    , m_browseJob( 0 )
    , m_createPlaylist( false )
{
}


RdioParser::~RdioParser()
{
}


void
RdioParser::parse( const QString& url )
{
    m_multi = false;
    m_total = 1;
    parseUrl( url );
}


void
RdioParser::parse( const QStringList& urls )
{
    m_multi = true;
    m_total = urls.count();

    foreach ( const QString& url, urls )
        parseUrl( url );
}


void
RdioParser::parseUrl( const QString& url )
{
    if ( url.contains( "rd.io" ) ) // shortened
    {
        ShortenedLinkParser* p = new ShortenedLinkParser( QStringList() << url, this );
        connect( p, SIGNAL( urls( QStringList ) ), this, SLOT( expandedLinks( QStringList ) ) );
        return;
    }

    if ( url.contains( "artist" ) && url.contains( "album" ) && url.contains( "track" ) )
        parseTrack( url );
    else
    {
        DropJob::DropType type = DropJob::None;
        if ( url.contains( "artist" ) && url.contains( "album" ) )
            type = DropJob::Album;
        else if ( url.contains( "artist" ) )
            type = DropJob::Artist;
        else if ( url.contains( "people" ) && url.contains( "playlist" ) )
            type = DropJob::Playlist;
        else
        {
            tLog() << "Got Rdio URL I can't parse!" << url;
            return;
        }

        // artist, album, or playlist link requre fetching
        fetchObjectsFromUrl( url, type );
    }
}


void
RdioParser::fetchObjectsFromUrl( const QString& url, DropJob::DropType type )
{
    QList< QPair< QByteArray, QByteArray > > params;
    params.append( QPair<QByteArray, QByteArray>( "extras", "tracks" ) );

    QString cleanedUrl = url;
    cleanedUrl.replace("#/", "");

    QByteArray data;
    QNetworkRequest request = generateRequest( "getObjectFromUrl", cleanedUrl, params, &data );

    request.setHeader( QNetworkRequest::ContentTypeHeader, QLatin1String( "application/x-www-form-urlencoded" ) );
    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->post( request, data ) );
    connect( reply, SIGNAL( finished() ), SLOT( rdioReturned() ) );

#ifndef ENABLE_HEADLESS
    m_browseJob = new DropJobNotifier( pixmap(), QString( "Rdio" ), type, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );
#endif

    m_reqQueries.insert( reply );
}


void
RdioParser::rdioReturned()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );
    m_reqQueries.remove( r );
    m_count++;
    r->deleteLater();

    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( r->reply(), &ok ).toMap();
        QVariantMap result = res.value( "result" ).toMap();

        if ( !ok || result.isEmpty() )
        {
            tLog() << "Failed to parse json from Rdio browse item:" << p.errorString() << "On line" << p.errorLine() << "With data:" << res;

            return;
        }

        QVariantList tracks = result.value( "tracks" ).toList();
        if ( tracks.isEmpty() )
        {
            tLog() << "Got no tracks in result, ignoring!" << result;
            return;
        }

        // Playlists will have these
        m_title = result[ "name" ].toString();
        m_creator = result[ "owner" ].toString();

        foreach( QVariant track, tracks )
        {
            QVariantMap rdioResult = track.toMap();
            QString title, artist, album;

            title = rdioResult.value( "name", QString() ).toString();
            artist = rdioResult.value( "artist", QString() ).toString();
            album = rdioResult.value( "album", QString() ).toString();

            if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
            {
                tLog() << "Didn't get an artist and track name from Rdio, not enough to build a query on. Aborting" << title << artist << album;
                return;
            }

            Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), !m_createPlaylist );
            if ( q.isNull() )
                continue;

            m_tracks << q;
        }
    }
    else
    {
#ifndef ENABLE_HEADLESS
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Error fetching Rdio information from the network!" ) ) );
#endif

        tLog() << "Error in network request to Rdio for track decoding:" << r->reply()->errorString();
    }

    checkFinished();
}


void
RdioParser::parseTrack( const QString& origUrl )
{
    QString url = origUrl;
    QString artist, trk, album, playlist;
    QString realUrl = url.replace( "_", " " );
    QString matchStr = "/%1/([^/]*)/";
    QString matchPlStr = "/%1/(?:[^/]*)/([^/]*)/";

    QRegExp r( QString( matchStr ).arg( "artist" ) );

    int loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        artist = r.cap( 1 );

    r = QRegExp( QString( matchStr ).arg( "album" ) );
    loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        album = r.cap( 1 );

    r = QRegExp( QString( matchStr ).arg( "track" ) );
    loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        trk = r.cap( 1 );

    r = QRegExp( QString( matchPlStr ).arg( "playlists" ) );
    loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        playlist = r.cap( 1 );

    if ( trk.isEmpty() || artist.isEmpty() )
    {
        tLog() << "Parsed Rdio track url but it's missing artist or track!" << url;
        return;
    }

    query_ptr q = Query::get( artist, trk, album, uuid(), !m_createPlaylist );
    m_count++;
    m_tracks << q;

    checkFinished();
}


QNetworkRequest
RdioParser::generateRequest( const QString& method, const QString& url, const QList< QPair< QByteArray, QByteArray > >& extraParams, QByteArray* data )
{
    QUrl fetchUrl( "http://api.rdio.com/1/" );
    QUrl toSignUrl = fetchUrl;


    QPair<QByteArray, QByteArray> param;
    foreach ( param, extraParams )
    {
        TomahawkUtils::urlAddQueryItem( toSignUrl, param.first, param.second );
    }
    TomahawkUtils::urlAddQueryItem( toSignUrl, "method", method );
    TomahawkUtils::urlAddQueryItem( toSignUrl, "oauth_consumer_key", "gk8zmyzj5xztt8aj48csaart" );
    QString nonce;
    for ( int i = 0; i < 8; i++ )
        nonce += QString::number( qrand() % 10 );
    TomahawkUtils::urlAddQueryItem( toSignUrl, "oauth_nonce", nonce );
    TomahawkUtils::urlAddQueryItem( toSignUrl, "oauth_signature_method", "HMAC-SHA1");
    TomahawkUtils::urlAddQueryItem( toSignUrl, "oauth_timestamp", QString::number(QDateTime::currentMSecsSinceEpoch() / 1000 ) );
    TomahawkUtils::urlAddQueryItem( toSignUrl, "oauth_version",  "1.0");
    TomahawkUtils::urlAddQueryItem( toSignUrl, "url", QUrl::toPercentEncoding( url ) );

    int size = TomahawkUtils::urlQueryItems( toSignUrl ).size();
    for( int i = 0; i < size; i++ ) {
        const QPair< QString, QString > item = TomahawkUtils::urlQueryItems( toSignUrl ).at( i );
        data->append( item.first + "=" + item.second + "&" );
    }
    data->truncate( data->size() - 1 ); // remove extra &

    QByteArray toSign = "POST&" + QUrl::toPercentEncoding( fetchUrl.toEncoded() ) + '&' + QUrl::toPercentEncoding( *data );
    qDebug() << "Rdio" << toSign;

    TomahawkUtils::urlAddQueryItem( toSignUrl, "oauth_signature", QUrl::toPercentEncoding( hmacSha1("yt35kakDyW&", toSign ) ) );

    data->clear();
    size = TomahawkUtils::urlQueryItems( toSignUrl ).size();
    for( int i = 0; i < size; i++ ) {
        const QPair< QString, QString > item = TomahawkUtils::urlQueryItems( toSignUrl ).at( i );
        data->append( item.first.toLatin1() + "=" + item.second.toLatin1() + "&" );
    }
    data->truncate( data->size() - 1 ); // remove extra &

    QNetworkRequest request = QNetworkRequest( fetchUrl );
    request.setHeader( QNetworkRequest::ContentTypeHeader, QLatin1String( "application/x-www-form-urlencoded" ) );

    return request;
}


QByteArray
RdioParser::hmacSha1(QByteArray key, QByteArray baseString)
{
#ifdef QCA2_FOUND
    QCA::MessageAuthenticationCode hmacsha1( "hmac(sha1)", QCA::SecureArray() );
    QCA::SymmetricKey keyObject( key );
    hmacsha1.setup( keyObject );

    hmacsha1.update( QCA::SecureArray( baseString ) );
    QCA::SecureArray resultArray = hmacsha1.final();

    QByteArray result = resultArray.toByteArray().toBase64();
    return result;
#else
    tLog() << "Tomahawk compiled without QCA support, cannot generate HMAC signature";
    return QByteArray();
#endif
}


void
RdioParser::checkFinished()
{
    tDebug() << "Checking for Rdio batch playlist job finished" << m_reqQueries.isEmpty();
    if ( m_reqQueries.isEmpty() ) // we're done
    {
        if ( m_browseJob )
            m_browseJob->setFinished();

        if ( m_tracks.isEmpty() )
            return;

        if ( m_createPlaylist )
        {
            m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                           uuid(),
                                           m_title,
                                           "",
                                           m_creator,
                                           false,
                                           m_tracks );

            connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );

            return;
        }
        else
        {
            if ( !m_multi )
                emit track( m_tracks.first() );
            else if ( m_multi && m_count == m_total )
                emit tracks( m_tracks );

            m_tracks.clear();
        }

        deleteLater();
    }
}


void
RdioParser::playlistCreated( Tomahawk::PlaylistRevision )
{
    ViewManager::instance()->show( m_playlist );
}


void
RdioParser::expandedLinks( const QStringList& urls )
{
    foreach( const QString& url, urls )
    {
        if ( url.contains( "rdio.com" ) || url.contains( "rd.io" ) )
            parseUrl( url );
    }
}


QPixmap
RdioParser::pixmap() const
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/rdio.png" );

    return *s_pixmap;
}
