/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2010-2012, Stefan Derkits <stefan@derkits.at>
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

#include "GroovesharkParser.h"

#include <QtCrypto>

#include <QCoreApplication>
#include <QtNetwork/QNetworkAccessManager>

#include <QWebPage>
#include <QWebFrame>
#include <QWebElement>

#include <qjson/parser.h>

#include "Query.h"
#include "SourceList.h"
#include "DropJob.h"
#include "DropJobNotifier.h"
#include "ViewManager.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;

QPixmap* GroovesharkParser::s_pixmap = 0;

const char* enApiSecret = "erCj5s0Vebyqtc9Aduyotc1CLListJ9HfO2os5hBeew=";

GroovesharkParser::GroovesharkParser( const QStringList& trackUrls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_limit ( 40 )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
{
    QByteArray magic = QByteArray::fromBase64( enApiSecret );

    QByteArray wand = QByteArray::fromBase64( QCoreApplication::applicationName().toLatin1() );
    int length = magic.length(), n2 = wand.length();
    for ( int i=0; i<length; i++ ) magic[i] = magic[i] ^ wand[i%n2];

    m_apiKey = QCA::SymmetricKey( magic );

    foreach ( const QString& url, trackUrls )
        lookupUrl( url );
}


GroovesharkParser::~GroovesharkParser()
{
}


void
GroovesharkParser::lookupUrl( const QString& link )
{
    if ( link.contains( "playlist" ) )
    {
        if ( !m_createNewPlaylist )
            m_trackMode = true;
        else
            m_trackMode = false;

        lookupGroovesharkPlaylist( link );
    }
    else if ( link.contains( "grooveshark.com/s/" ) || link.contains( "grooveshark.com/#/s/" ) )
        lookupGroovesharkTrack( link );
    else
        return;
}


void
GroovesharkParser::lookupGroovesharkPlaylist( const QString& linkRaw )
{
    tLog() << "Parsing Grooveshark Playlist URI:" << linkRaw;

    QString urlFragment = QUrl( linkRaw ).fragment( );
    if ( urlFragment.isEmpty() ) {
        tDebug() << "no fragment, setting fragment to path";
        urlFragment = QUrl(linkRaw).path();
    }

    int paramStartingPostition = urlFragment.indexOf( "?" );
    if ( paramStartingPostition != -1 )
        urlFragment.truncate( paramStartingPostition );

    QStringList urlParts = urlFragment.split( "/", QString::SkipEmptyParts );

    bool ok;
    int playlistID = urlParts.at( 2 ).toInt( &ok, 10 );
    if ( !ok )
    {
        tDebug() << "Incorrect grooveshark url";
        return;
    }

    m_title = urlParts.at( 1 );
    DropJob::DropType type;
    type = DropJob::Playlist;
    QString base_url( "http://api.grooveshark.com/ws3.php?sig=" );
    QByteArray data = QString( "{\"method\":\"getPlaylistSongs\",\"parameters\":{\"playlistID\":\"%1\"},\"header\":{\"wsKey\":\"tomahawkplayer\"}}" ).arg( playlistID ).toLocal8Bit();

    QCA::MessageAuthenticationCode hmac( "hmac(md5)", m_apiKey );

    QCA::SecureArray secdata( data );
    hmac.update(secdata);
    QCA::SecureArray resultArray = hmac.final();

    QString hash = QCA::arrayToHex( resultArray.toByteArray() );
    QUrl url = QUrl( base_url + hash );

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->post( QNetworkRequest( url ), data ) );
    connect( reply, SIGNAL( finished() ), SLOT( groovesharkLookupFinished() ) );

#ifndef ENABLE_HEADLESS
    m_browseJob = new DropJobNotifier( pixmap(), "Grooveshark", type, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );
#endif

    m_queries.insert( reply );
}


void
GroovesharkParser::lookupGroovesharkTrack( const QString& track )
{
    tLog() << "Parsing Grooveshark Track Page:" << track;

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( QUrl( track ) ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( trackPageFetchFinished() ) );

#ifndef ENABLE_HEADLESS
    m_browseJob = new DropJobNotifier( pixmap(), "Grooveshark", DropJob::Track, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );
#endif

    m_queries << reply;
}


void
GroovesharkParser::trackPageFetchFinished()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );

    m_queries.remove( r );
    r->deleteLater();

    QWebPage page;
    page.settings()->setAttribute( QWebSettings::JavascriptEnabled, false );
    page.settings()->setAttribute( QWebSettings::PluginsEnabled, false );
    page.settings()->setAttribute( QWebSettings::JavaEnabled, false );
    page.settings()->setAttribute( QWebSettings::AutoLoadImages, false );
    page.mainFrame()->setHtml( QString::fromUtf8( r->reply()->readAll() ) );
    QWebElement title = page.mainFrame()->findFirstElement("span[itemprop='name']");
    QWebElement artist = page.mainFrame()->findFirstElement("noscript span[itemprop='byArtist']");
    QWebElement album = page.mainFrame()->findFirstElement("noscript span[itemprop='inAlbum']");

    if ( !title.toPlainText().isEmpty() && !artist.toPlainText().isEmpty() )
    {
        tDebug() << "Got track info from grooveshark, enough to create a query:" << title.toPlainText() << artist.toPlainText() << album.toPlainText();

        Tomahawk::query_ptr q = Tomahawk::Query::get( artist.toPlainText(), title.toPlainText(), album.toPlainText(), uuid(), true );
        if ( !q.isNull() )
            m_tracks << q;
    }

    checkTrackFinished();
}


void
GroovesharkParser::groovesharkLookupFinished()
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
            tLog() << "Failed to parse json from Grooveshark browse item:" << p.errorString() << "On line" << p.errorLine();
            checkTrackFinished();
            return;
        }

        QVariantList list = res.value( "result" ).toMap().value( "songs" ).toList();
        foreach ( const QVariant& var, list )
        {
            QVariantMap trackResult = var.toMap();

            QString title, artist, album;

            title = trackResult.value( "SongName", QString() ).toString();
            artist = trackResult.value( "ArtistName", QString() ).toString();
            album = trackResult.value( "AlbumName", QString() ).toString();

            if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
            {
                tLog() << "Didn't get an artist and track name from grooveshark, not enough to build a query on. Aborting" << title << artist << album;
                return;
            }

            Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), m_trackMode );
            m_tracks << q;
        }
    }
    else
    {
#ifndef ENABLE_HEADLESS
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Error fetching Grooveshark information from the network!" ) ) );
#endif

        tLog() << "Error in network request to grooveshark for track decoding:" << r->reply()->errorString();
    }

    if ( m_trackMode )
        checkTrackFinished();
    else
        checkPlaylistFinished();
}


void
GroovesharkParser::checkPlaylistFinished()
{
    tDebug() << "Checking for grooveshark batch playlist job finished" << m_queries.isEmpty() << m_createNewPlaylist;
    if ( m_queries.isEmpty() ) // we're done
    {
        if ( m_browseJob )
            m_browseJob->setFinished();

        if( m_createNewPlaylist && !m_tracks.isEmpty() )
        {
            m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       m_title,
                                       m_info,
                                       m_creator,
                                       false,
                                       m_tracks );
            connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );
            return;
        }

        emit tracks( m_tracks );
        deleteLater();
    }
}


void
GroovesharkParser::checkTrackFinished()
{
    tDebug() << "Checking for grooveshark batch track job finished" << m_queries.isEmpty();
    if ( m_queries.isEmpty() ) // we're done
    {
        if ( m_browseJob )
            m_browseJob->setFinished();

        emit tracks( m_tracks );

        deleteLater();
    }
}


void
GroovesharkParser::playlistCreated()
{
    ViewManager::instance()->show( m_playlist );

    deleteLater();
}


QPixmap
GroovesharkParser::pixmap() const
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/grooveshark.png" );

    return *s_pixmap;
}
