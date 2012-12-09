/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
    Copyright (C) 2011, Jeff Mitchell <jeff@tomahawk-player.org>
    Copyright (C) 2011-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "GlobalActionManager.h"

#include "Artist.h"
#include "Album.h"
#include "SourceList.h"
#include "Pipeline.h"
#include "TomahawkSettings.h"
#include "audio/AudioEngine.h"
#include "database/LocalCollection.h"
#include "playlist/dynamic/GeneratorInterface.h"

#include "echonest/Playlist.h"

#include "utils/XspfLoader.h"
#include "utils/XspfGenerator.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"

#include "utils/JspfLoader.h"
#include "utils/SpotifyParser.h"
#include "utils/ShortenedLinkParser.h"
#include "utils/RdioParser.h"

#ifndef ENABLE_HEADLESS
    #include "ViewManager.h"
    #include "playlist/PlaylistView.h"
    #include "widgets/SearchWidget.h"

    #include <QtGui/QApplication>
    #include <QtGui/QClipboard>
#endif

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkConfiguration>
#include <QtNetwork/QNetworkProxy>

#include <QtCore/QMimeData>
#include <QtCore/QUrl>


GlobalActionManager* GlobalActionManager::s_instance = 0;

using namespace Tomahawk;


GlobalActionManager*
GlobalActionManager::instance()
{
    if ( !s_instance )
        s_instance = new GlobalActionManager;

    return s_instance;
}


GlobalActionManager::GlobalActionManager( QObject* parent )
    : QObject( parent )
{
}


GlobalActionManager::~GlobalActionManager()
{
}


QUrl
GlobalActionManager::openLinkFromQuery( const query_ptr& query ) const
{
    QString title = query->displayQuery()->track();
    QString artist = query->displayQuery()->artist();
    QString album = query->displayQuery()->album();

    return openLink( title, artist, album );
}


QUrl
GlobalActionManager::copyOpenLink( const artist_ptr& artist ) const
{
    const QUrl link( QString( "%1/artist/%2" ).arg( hostname() ).arg( artist->name() ) );

    QClipboard* cb = QApplication::clipboard();
    QByteArray data = percentEncode( link );
    cb->setText( data );

    return link;
}


QUrl
GlobalActionManager::copyOpenLink( const album_ptr& album ) const
{
    const QUrl link = QUrl::fromUserInput( QString( "%1/album/%2/%3" ).arg( hostname() ).arg( album->artist().isNull() ? QString() : album->artist()->name() ).arg( album->name() ) );

    QClipboard* cb = QApplication::clipboard();
    QByteArray data = percentEncode( link );

    cb->setText( data );

    return link;
}


QUrl
GlobalActionManager::openLink( const QString& title, const QString& artist, const QString& album ) const
{
    QUrl link( QString( "%1/open/track/" ).arg( hostname() ) );

    if ( !artist.isEmpty() )
        link.addQueryItem( "artist", artist );
    if ( !title.isEmpty() )
        link.addQueryItem( "title", title );
    if ( !album.isEmpty() )
        link.addQueryItem( "album", album );

    return link;
}


void
GlobalActionManager::shortenLink( const QUrl& url, const QVariant& callbackObj )
{
    tDebug() << Q_FUNC_INFO << "callbackObj is valid: " << ( callbackObj.isValid() ? "true" : "false" );
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "shortenLink", Qt::QueuedConnection, Q_ARG( QUrl, url ), Q_ARG( QVariant, callbackObj ) );
        return;
    }

    QNetworkRequest request;
    request.setUrl( url );

    qDebug() << "Doing lookup:" << url.toEncoded();
    QNetworkReply *reply = TomahawkUtils::nam()->get( request );
    if ( callbackObj.isValid() )
        reply->setProperty( "callbackobj", callbackObj );
    connect( reply, SIGNAL( finished() ), SLOT( shortenLinkRequestFinished() ) );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( shortenLinkRequestError( QNetworkReply::NetworkError ) ) );
}


#ifndef ENABLE_HEADLESS

void
GlobalActionManager::getShortLink( const playlist_ptr& pl )
{
    QVariantMap m;
    m[ "title" ] = pl->title();
    m[ "creator" ] = pl->author().isNull() ? "" : pl->author()->friendlyName();
    QVariantList tracks;
    foreach( const plentry_ptr& pl, pl->entries() )
    {
        if ( pl->query().isNull() )
            continue;

        QVariantMap track;
        track[ "title" ] = pl->query()->track();
        track[ "creator" ] = pl->query()->artist();
        track[ "album" ] = pl->query()->album();

        tracks << track;
    }
    m[ "track" ] = tracks;

    QVariantMap jspf;
    jspf["playlist"] = m;

    QJson::Serializer s;
    QByteArray msg = s.serialize( jspf );

    // No built-in Qt facilities for doing a FORM POST. So we build the payload ourselves...
    const QByteArray boundary = "----------------------------2434992cccab";
    QByteArray data( QByteArray( "--" + boundary + "\r\n" ) );
    data += "Content-Disposition: form-data; name=\"data\"; filename=\"playlist.jspf\"\r\n";
    data += "Content-Type: application/octet-stream\r\n\r\n";
    data += msg;
    data += "\r\n\r\n";
    data += "--" + boundary + "--\r\n\r\n";

    const QUrl url( QString( "%1/p/").arg( hostname() ) );
    QNetworkRequest req( url );
    req.setHeader( QNetworkRequest::ContentTypeHeader, QString( "multipart/form-data; boundary=%1" ).arg( QString::fromLatin1( boundary ) ) );
    QNetworkReply *reply = TomahawkUtils::nam()->post( req, data );

    connect( reply, SIGNAL( finished() ), SLOT( postShortenFinished() ) );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( shortenLinkRequestError( QNetworkReply::NetworkError ) ) );
}


QString
GlobalActionManager::copyPlaylistToClipboard( const dynplaylist_ptr& playlist )
{
    QUrl link( QString( "%1/%2/create/" ).arg( hostname() ).arg( playlist->mode() == OnDemand ? "station" : "autoplaylist" ) );

    if ( playlist->generator()->type() != "echonest" )
    {
        tLog() << "Only echonest generators are supported";
        return QString();
    }

    link.addEncodedQueryItem( "type", "echonest" );
    link.addQueryItem( "title", playlist->title() );

    QList< dyncontrol_ptr > controls = playlist->generator()->controls();
    foreach ( const dyncontrol_ptr& c, controls )
    {
        if ( c->selectedType() == "Artist" )
        {
            if ( c->match().toInt() == Echonest::DynamicPlaylist::ArtistType )
                link.addQueryItem( "artist_limitto", c->input() );
            else
                link.addQueryItem( "artist", c->input() );
        }
        else if ( c->selectedType() == "Artist Description" )
        {
            link.addQueryItem( "description", c->input() );
        }
        else
        {
            QString name = c->selectedType().toLower().replace( " ", "_" );
            Echonest::DynamicPlaylist::PlaylistParam p = static_cast< Echonest::DynamicPlaylist::PlaylistParam >( c->match().toInt() );
            // if it is a max, set that too
            if ( p == Echonest::DynamicPlaylist::MaxTempo || p == Echonest::DynamicPlaylist::MaxDuration || p == Echonest::DynamicPlaylist::MaxLoudness
               || p == Echonest::DynamicPlaylist::MaxDanceability || p == Echonest::DynamicPlaylist::MaxEnergy || p == Echonest::DynamicPlaylist::ArtistMaxFamiliarity
               || p == Echonest::DynamicPlaylist::ArtistMaxHotttnesss || p == Echonest::DynamicPlaylist::SongMaxHotttnesss || p == Echonest::DynamicPlaylist::ArtistMaxLatitude
               || p == Echonest::DynamicPlaylist::ArtistMaxLongitude )
                name += "_max";

            link.addQueryItem( name, c->input() );
        }
    }

    QClipboard* cb = QApplication::clipboard();
    QByteArray data = percentEncode( link );
    cb->setText( data );

    return link.toString();
}


void
GlobalActionManager::savePlaylistToFile( const playlist_ptr& playlist, const QString& filename )
{
    XSPFGenerator* g = new XSPFGenerator( playlist, this );
    g->setProperty( "filename", filename );

    connect( g, SIGNAL( generated( QByteArray ) ), this, SLOT( xspfCreated( QByteArray ) ) );
}


void
GlobalActionManager::xspfCreated( const QByteArray& xspf )
{
    QString filename = sender()->property( "filename" ).toString();

    QFile f( filename );
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        qWarning() << "Failed to open file to save XSPF:" << filename;
        return;
    }

    f.write( xspf );
    f.close();

    sender()->deleteLater();
}


void
GlobalActionManager::copyToClipboard( const query_ptr& query )
{
    m_clipboardLongUrl = openLinkFromQuery( query );
    shortenLink( m_clipboardLongUrl );
}


bool
GlobalActionManager::parseTomahawkLink( const QString& urlIn )
{
    QString url = urlIn;
    if ( urlIn.startsWith( "http://toma.hk" ) )
        url.replace( "http://toma.hk/", "tomahawk://" );

    if ( url.contains( "tomahawk://" ) )
    {
        QString cmd = url.mid( 11 );
        cmd.replace( "%2B", "%20" );
        cmd.replace( "+", "%20" ); // QUrl doesn't parse '+' into " "
        tLog() << "Parsing tomahawk link command" << cmd;

        QString cmdType = cmd.split( "/" ).first();
        QUrl u = QUrl::fromEncoded( cmd.toUtf8() );

        // for backwards compatibility
        if ( cmdType == "load" )
        {
            if ( u.hasQueryItem( "xspf" ) )
            {
                QUrl xspf = QUrl::fromUserInput( u.queryItemValue( "xspf" ) );
                XSPFLoader* l = new XSPFLoader( true, this );
                tDebug() << "Loading spiff:" << xspf.toString();
                l->load( xspf );
                connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), ViewManager::instance(), SLOT( show( Tomahawk::playlist_ptr ) ) );

                return true;
            }
            else if ( u.hasQueryItem( "jspf" ) )
            {
                QUrl jspf = QUrl::fromUserInput( u.queryItemValue( "jspf" ) );
                JSPFLoader* l = new JSPFLoader( true, this );

                tDebug() << "Loading jspiff:" << jspf.toString();
                l->load( jspf );
                connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), ViewManager::instance(), SLOT( show( Tomahawk::playlist_ptr ) ) );

                return true;
            }
        }

        if ( cmdType == "playlist" )
        {
            return handlePlaylistCommand( u );
        }
        else if ( cmdType == "collection" )
        {
            return handleCollectionCommand( u );
        }
        else if ( cmdType == "queue" )
        {
            return handleQueueCommand( u );
        }
        else if ( cmdType == "station" )
        {
            return handleStationCommand( u );
        }
        else if ( cmdType == "autoplaylist" )
        {
            return handleAutoPlaylistCommand( u );
        }
        else if ( cmdType == "search" )
        {
            return handleSearchCommand( u );
        }
        else if ( cmdType == "play" )
        {
            return handlePlayCommand( u );
        }
        else if ( cmdType == "bookmark" )
        {
            return handlePlayCommand( u );
        }
        else if ( cmdType == "open" )
        {
            return handleOpenCommand( u );
        }
        else if ( cmdType == "view" )
        {
            return handleViewCommand( u );
        }
        else if ( cmdType == "import" )
        {
            return handleImportCommand( u );
        }
        else
        {
            tLog() << "Tomahawk link not supported, command not known!" << cmdType << u.path();
            return false;
        }
    }
    else
    {
        tLog() << "Not a tomahawk:// link!";
        return false;
    }
}


bool
GlobalActionManager::handlePlaylistCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific playlist command:" << url.toString();
        return false;
    }

    if ( parts[ 0 ] == "import" )
    {
        if ( !url.hasQueryItem( "xspf" ) && !url.hasQueryItem( "jspf") )
        {
            tDebug() << "No xspf or jspf to load...";
            return false;
        }
        if ( url.hasQueryItem( "xspf" ) )
        {
            createPlaylistFromUrl( "xspf", url.queryItemValue( "xspf" ), url.hasQueryItem( "title" ) ? url.queryItemValue( "title" ) : QString() );
            return true;
        }
        else if ( url.hasQueryItem( "jspf" ) )
        {
            createPlaylistFromUrl( "jspf", url.queryItemValue( "jspf" ), url.hasQueryItem( "title" ) ? url.queryItemValue( "title" ) : QString() );
            return true;
        }
    }
    else if ( parts [ 0 ] == "new" )
    {
        if ( !url.hasQueryItem( "title" ) )
        {
            tLog() << "New playlist command needs a title...";
            return false;
        }
        playlist_ptr pl = Playlist::create( SourceList::instance()->getLocal(), uuid(), url.queryItemValue( "title" ), QString(), QString(), false );
        ViewManager::instance()->show( pl );
    }
    else if ( parts[ 0 ] == "add" )
    {
        if ( !url.hasQueryItem( "playlistid" ) || !url.hasQueryItem( "title" ) || !url.hasQueryItem( "artist" ) )
        {
            tLog() << "Add to playlist command needs playlistid, track, and artist..." << url.toString();
            return false;
        }
        // TODO implement. Let the user select what playlist to add to
        return false;
    }

    return false;
}


bool
GlobalActionManager::handleImportCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.size() < 1 )
        return false;

    if ( parts[ 0 ] == "playlist" )
    {
        if ( url.hasQueryItem( "xspf" ) )
        {
            createPlaylistFromUrl( "xspf", url.queryItemValue( "xspf" ), url.hasQueryItem( "title" ) ? url.queryItemValue( "title" ) : QString() );
            return true;
        }
        else if ( url.hasQueryItem( "jspf" ) )
        {
            createPlaylistFromUrl( "jspf", url.queryItemValue( "jspf" ), url.hasQueryItem( "title" ) ? url.queryItemValue( "title" ) : QString() );
            return true;
        }
    }

    return false;
}


void
GlobalActionManager::createPlaylistFromUrl( const QString& type, const QString &url, const QString& title )
{
    if ( type == "xspf" )
    {
        QUrl xspf = QUrl::fromUserInput( url );
        XSPFLoader* l= new XSPFLoader( true, this );
        l->setOverrideTitle( title );
        l->load( xspf );
        connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), this, SLOT( playlistCreatedToShow( Tomahawk::playlist_ptr) ) );
    }
    else if ( type == "jspf" )
    {
        QUrl jspf = QUrl::fromUserInput( url );
        JSPFLoader* l= new JSPFLoader( true, this );
        l->setOverrideTitle( title );
        l->load( jspf );
        connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), this, SLOT( playlistCreatedToShow( Tomahawk::playlist_ptr) ) );
    }
}


void
GlobalActionManager::playlistCreatedToShow( const playlist_ptr& pl )
{
    connect( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistReadyToShow() ) );
    pl->setProperty( "sharedptr", QVariant::fromValue<Tomahawk::playlist_ptr>( pl ) );
}


void
GlobalActionManager::playlistReadyToShow()
{
    playlist_ptr pl = sender()->property( "sharedptr" ).value<Tomahawk::playlist_ptr>();
    if ( !pl.isNull() )
        ViewManager::instance()->show( pl );

    disconnect( sender(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistReadyToShow() ) );
}


bool
GlobalActionManager::handleCollectionCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific collection command:" << url.toString();
        return false;
    }

    if ( parts[ 0 ] == "add" )
    {
        // TODO implement
    }

    return false;
}


bool
GlobalActionManager::handleOpenCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 );
    if ( parts.isEmpty() )
    {
        tLog() << "No specific type to open:" << url.toString();
        return false;
    }
    // TODO user configurable in the UI
    return doQueueAdd( parts, url.queryItems() );
}


void
GlobalActionManager::handleOpenTrack( const query_ptr& q )
{
    ViewManager::instance()->queue()->model()->appendQuery( q );
    ViewManager::instance()->showQueue();

    if ( !AudioEngine::instance()->isPlaying() && !AudioEngine::instance()->isPaused() )
    {
        connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( waitingForResolved( bool ) ) );
        m_waitingToPlay = q;
    }
}


void
GlobalActionManager::handleOpenTracks( const QList< query_ptr >& queries )
{
    if ( queries.isEmpty() )
        return;

    ViewManager::instance()->queue()->model()->appendQueries( queries );
    ViewManager::instance()->showQueue();

    if ( !AudioEngine::instance()->isPlaying() && !AudioEngine::instance()->isPaused() )
    {
        connect( queries.first().data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( waitingForResolved( bool ) ) );
        m_waitingToPlay = queries.first();
    }
}


void
GlobalActionManager::handlePlayTrack( const query_ptr& qry )
{
    playNow( qry );
}


bool
GlobalActionManager::handleQueueCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific queue command:" << url.toString();
        return false;
    }

    if ( parts[ 0 ] == "add" )
    {
        doQueueAdd( parts.mid( 1 ), url.queryItems() );
    }
    else
    {
        tLog() << "Only queue/add/track is support at the moment, got:" << parts;
        return false;
    }

    return false;
}


bool
GlobalActionManager::doQueueAdd( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems )
{
    if ( parts.size() && parts[ 0 ] == "track" )
    {
        if ( queueSpotify( parts, queryItems ) )
            return true;
        else if ( queueRdio( parts, queryItems ) )
            return true;

        QPair< QString, QString > pair;
        QString title, artist, album, urlStr;
        foreach ( pair, queryItems )
        {
            pair.second = pair.second.replace( "+", " " ); // QUrl::queryItems doesn't decode + to a space :(
            if ( pair.first == "title" )
                title = pair.second;
            else if ( pair.first == "artist" )
                artist = pair.second;
            else if ( pair.first == "album" )
                album = pair.second;
            else if ( pair.first == "url" )
                urlStr = pair.second;
        }

        if ( !title.isEmpty() || !artist.isEmpty() || !album.isEmpty() )
        {
            // an individual; query to add to queue
            query_ptr q = Query::get( artist, title, album, uuid(), false );
            if ( q.isNull() )
                return false;

            if ( !urlStr.isEmpty() )
            {
                q->setResultHint( urlStr );
                q->setSaveHTTPResultHint( true );
            }

            Pipeline::instance()->resolve( q, true );

            handleOpenTrack( q );
            return true;
        }
        else
        { // a list of urls to add to the queue
            foreach ( pair, queryItems )
            {
                if ( pair.first != "url" )
                    continue;
                QUrl track = QUrl::fromUserInput( pair.second );
                //FIXME: isLocalFile is Qt 4.8
                if ( track.toString().startsWith( "file://" ) )
                {
                    // it's local, so we see if it's in the DB and load it if so
                    // TODO
                }
                else
                { // give it a web result hint
                    QFileInfo info( track.path() );
                    query_ptr q = Query::get( QString(), info.baseName(), QString(), uuid(), false );

                    if ( q.isNull() )
                        continue;

                    q->setResultHint( track.toString() );
                    q->setSaveHTTPResultHint( true );


                    q->setResultHint( track.toString() );
                    Pipeline::instance()->resolve( q );

                    ViewManager::instance()->queue()->model()->appendQuery( q );
                    ViewManager::instance()->showQueue();
                }
                return true;
            }
        }
    }
    else if ( parts.size() && parts[ 0 ] == "playlist" )
    {
        QString xspfUrl, jspfUrl;
        for ( int i = 0; i < queryItems.size(); i++ )
        {
            const QPair< QString, QString > queryItem = queryItems.at( i );
            if ( queryItem.first == "xspf" )
            {
                xspfUrl = queryItem.second;
                break;
            }
            else if ( queryItem.first == "jspf" )
            {
                jspfUrl = queryItem.second;
                break;
            }
        }

        if ( !xspfUrl.isEmpty() )
        {
            XSPFLoader* loader = new XSPFLoader( false, false, this );
            connect( loader, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( handleOpenTracks( QList< Tomahawk::query_ptr > ) ) );
            loader->load( QUrl( xspfUrl ) );
            loader->setAutoDelete( true );

            return true;
        }
        else if ( !jspfUrl.isEmpty() )
        {
            JSPFLoader* loader = new JSPFLoader( false, this );
            connect( loader, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( handleOpenTracks( QList< Tomahawk::query_ptr > ) ) );
            loader->load( QUrl( jspfUrl ) );
            loader->setAutoDelete( true );

            return true;
        }
    }
    return false;
}


bool
GlobalActionManager::queueSpotify( const QStringList& , const QList< QPair< QString, QString > >& queryItems )
{
    QString url;

    QPair< QString, QString > pair;
    foreach ( pair, queryItems )
    {
        if ( pair.first == "spotifyURL" )
            url = pair.second;
        else if ( pair.first == "spotifyURI" )
            url = pair.second;
    }

    if ( url.isEmpty() )
        return false;

    openSpotifyLink( url );

    return true;
}


bool
GlobalActionManager::queueRdio( const QStringList& , const QList< QPair< QString, QString > >& queryItems )
{
    QString url;

    QPair< QString, QString > pair;
    foreach ( pair, queryItems )
    {
        if ( pair.first == "rdioURL" )
            url = pair.second;
        else if ( pair.first == "rdioURI" )
            url = pair.second;
    }

    if ( url.isEmpty() )
        return false;

    openRdioLink( url );

    return true;
}


bool
GlobalActionManager::handleSearchCommand( const QUrl& url )
{
    // open the super collection and set this as the search filter
    QString queryStr;
    if ( url.hasQueryItem( "query" ) )
        queryStr = url.queryItemValue( "query" );
    else
    {
        QStringList query;
        if ( url.hasQueryItem( "artist" ) )
            query << url.queryItemValue( "artist" );
        if ( url.hasQueryItem( "album" ) )
            query << url.queryItemValue( "album" );
        if ( url.hasQueryItem( "title" ) )
            query << url.queryItemValue( "title" );
        queryStr = query.join( " " );
    }

    if ( queryStr.trimmed().isEmpty() )
        return false;

    ViewManager::instance()->show( new SearchWidget( queryStr.trimmed() ) );

    return true;
}

bool
GlobalActionManager::handleViewCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific view command:" << url.toString();
        return false;
    }

    if ( parts[ 0 ] == "artist" )
    {
        const QString artist = QUrl::fromPercentEncoding( url.encodedQueryItemValue( "name" ) ).replace( "+", " " );
        if ( artist.isEmpty() )
        {
            tLog() << "No artist supplied for view/artist command.";
            return false;
        }

        artist_ptr artistPtr = Artist::get( artist );
        if ( !artistPtr.isNull() )
            ViewManager::instance()->show( artistPtr );

        return true;
    }
    else if ( parts[ 0 ] == "album" )
    {
        const QString artist = QUrl::fromPercentEncoding( url.encodedQueryItemValue( "artist" ) ).replace( "+", " " );
        const QString album = QUrl::fromPercentEncoding( url.encodedQueryItemValue( "name" ) ).replace( "+", " " );
        if ( artist.isEmpty() || album.isEmpty() )
        {
            tLog() << "No artist or album supplied for view/album command:" << url;
            return false;
        }

        album_ptr albumPtr = Album::get( Artist::get( artist, false ), album, false );
        if ( !albumPtr.isNull() )
            ViewManager::instance()->show( albumPtr );

        return true;
    }
    else if ( parts[ 0 ] == "track" )
    {
        const QString artist = QUrl::fromPercentEncoding( url.encodedQueryItemValue( "artist" ) ).replace( "+", " " );
        const QString album = QUrl::fromPercentEncoding( url.encodedQueryItemValue( "album" ) ).replace( "+", " " );
        const QString track = QUrl::fromPercentEncoding( url.encodedQueryItemValue( "name" ) ).replace( "+", " " );
        if ( artist.isEmpty() || track.isEmpty() )
        {
            tLog() << "No artist or track supplied for view/track command:" << url;
            return false;
        }

        query_ptr queryPtr = Query::get( artist, track, album );
        if ( !queryPtr.isNull() )
            ViewManager::instance()->show( queryPtr );

        return true;
    }

    return false;
}


bool
GlobalActionManager::handleAutoPlaylistCommand( const QUrl& url )
{
    return !loadDynamicPlaylist( url, false ).isNull();
}


Tomahawk::dynplaylist_ptr
GlobalActionManager::loadDynamicPlaylist( const QUrl& url, bool station )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific station command:" << url.toString();
        return Tomahawk::dynplaylist_ptr();
    }

    if ( parts[ 0 ] == "create" )
    {
        if ( !url.hasQueryItem( "title" ) || !url.hasQueryItem( "type" ) )
        {
            tLog() << "Station create command needs title and type..." << url.toString();
            return Tomahawk::dynplaylist_ptr();
        }
        QString title = url.queryItemValue( "title" );
        QString type = url.queryItemValue( "type" );
        GeneratorMode m = Static;
        if ( station )
            m = OnDemand;

        dynplaylist_ptr pl = DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), title, QString(), QString(), m, false, type );
        pl->setMode( m );
        QList< dyncontrol_ptr > controls;
        QPair< QString, QString > param;
        foreach ( param, url.queryItems() )
        {
            if ( param.first == "artist" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Artist" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistRadioType ) );
                controls << c;
            }
            else if ( param.first == "artist_limitto" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Artist" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistType ) );
                controls << c;
            }
            else if ( param.first == "description" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Artist Description" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistDescriptionType ) );
                controls << c;
            }
            else if ( param.first == "variety" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Variety" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::Variety ) );
                controls << c;
            }
            else if ( param.first.startsWith( "tempo" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Tempo" );
                int extra = param.first.endsWith( "_max" ) ? -1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::MinTempo + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "duration" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Duration" );
                int extra = param.first.endsWith( "_max" ) ? -1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::MinDuration + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "loudness" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Loudness" );
                int extra = param.first.endsWith( "_max" ) ? -1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::MinLoudness + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "danceability" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Danceability" );
                int extra = param.first.endsWith( "_max" ) ? 1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::MinDanceability + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "energy" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Energy" );
                int extra = param.first.endsWith( "_max" ) ? 1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::MinEnergy + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "artist_familiarity" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Artist Familiarity" );
                int extra = param.first.endsWith( "_max" ) ? -1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistMinFamiliarity + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "artist_hotttnesss" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Artist Hotttnesss" );
                int extra = param.first.endsWith( "_max" ) ? -1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistMinHotttnesss + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "song_hotttnesss" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Song Hotttnesss" );
                int extra = param.first.endsWith( "_max" ) ? -1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::SongMinHotttnesss + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "longitude" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Longitude" );
                int extra = param.first.endsWith( "_max" ) ? 1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistMinLongitude + extra ) );
                controls << c;
            }
            else if ( param.first.startsWith( "latitude" ) )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Latitude" );
                int extra = param.first.endsWith( "_max" ) ? 1 : 0;
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistMinLatitude + extra ) );
                controls << c;
            }
            else if ( param.first == "key" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Key" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::Key ) );
                controls << c;
            }
            else if ( param.first == "mode" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Mode" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::Mode ) );
                controls << c;
            }
            else if ( param.first == "mood" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Mood" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::Mood ) );
                controls << c;
            }
            else if ( param.first == "style" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Style" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::Style ) );
                controls << c;
            }
            else if ( param.first == "song" )
            {
                dyncontrol_ptr c = pl->generator()->createControl( "Song" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::SongRadioType ) );
                controls << c;
            }
        }

        if ( m == OnDemand )
            pl->createNewRevision( uuid(), pl->currentrevision(), type, controls );
        else
            pl->createNewRevision( uuid(), pl->currentrevision(), type, controls, pl->entries() );

        ViewManager::instance()->show( pl );
        return pl;
    }

    return Tomahawk::dynplaylist_ptr();
}


bool
GlobalActionManager::handleStationCommand( const QUrl& url )
{
    return !loadDynamicPlaylist( url, true ).isNull();
}


bool
GlobalActionManager::handlePlayCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific play command:" << url.toString();
        return false;
    }

    if ( parts[ 0 ] == "track" )
    {
        if ( playSpotify( url ) )
            return true;
        else if ( playRdio( url ) )
            return true;

        QPair< QString, QString > pair;
        QString title, artist, album, urlStr;
        foreach ( pair, url.queryItems() )
        {
            if ( pair.first == "title" )
                title = pair.second;
            else if ( pair.first == "artist" )
                artist = pair.second;
            else if ( pair.first == "album" )
                album = pair.second;
            else if ( pair.first == "url" )
                urlStr = pair.second;
        }

        query_ptr q = Query::get( artist, title, album );
        if ( q.isNull() )
            return false;

        if ( !urlStr.isEmpty() )
        {
            q->setResultHint( urlStr );
            q->setSaveHTTPResultHint( true );
        }

        playNow( q );
        return true;
    }

    return false;
}


bool
GlobalActionManager::playSpotify( const QUrl& url )
{
    if ( !url.hasQueryItem( "spotifyURI" ) && !url.hasQueryItem( "spotifyURL" ) )
        return false;

    QString spotifyUrl = url.hasQueryItem( "spotifyURI" ) ? url.queryItemValue( "spotifyURI" ) : url.queryItemValue( "spotifyURL" );
    SpotifyParser* p = new SpotifyParser( spotifyUrl, this );
    connect( p, SIGNAL( track( Tomahawk::query_ptr ) ), this, SLOT( playOrQueueNow( Tomahawk::query_ptr ) ) );

    return true;
}


void
GlobalActionManager::playNow( const query_ptr& q )
{
    Pipeline::instance()->resolve( q, true );

    m_waitingToPlay = q;
    q->setProperty( "playNow", true );
    connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( waitingForResolved( bool ) ) );
}


void
GlobalActionManager::playOrQueueNow( const query_ptr& q )
{
    Pipeline::instance()->resolve( q, true );

    m_waitingToPlay = q;
    connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( waitingForResolved( bool ) ) );
}


bool
GlobalActionManager::playRdio( const QUrl& url )
{
    if ( !url.hasQueryItem( "rdioURI" ) && !url.hasQueryItem( "rdioURL" ) )
        return false;

    QString rdioUrl = url.hasQueryItem( "rdioURI" ) ? url.queryItemValue( "spotifyURI" ) : url.queryItemValue( "rdioURL" );
    RdioParser* p = new RdioParser( this );
    p->parse( rdioUrl );
    connect( p, SIGNAL( track( Tomahawk::query_ptr ) ), this, SLOT( playOrQueueNow( Tomahawk::query_ptr ) ) );

    return true;
}

#endif


bool GlobalActionManager::handleBookmarkCommand(const QUrl& url)
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific bookmark command:" << url.toString();
        return false;
    }

    if ( parts[ 0 ] == "track" )
    {
        QPair< QString, QString > pair;
        QString title, artist, album, urlStr;
        foreach ( pair, url.queryItems() )
        {
            if ( pair.first == "title" )
                title = pair.second;
            else if ( pair.first == "artist" )
                artist = pair.second;
            else if ( pair.first == "album" )
                album = pair.second;
            else if ( pair.first == "url" )
                urlStr = pair.second;
        }
        query_ptr q = Query::get( artist, title, album );
        if ( q.isNull() )
            return false;

        if ( !urlStr.isEmpty() )
        {
            q->setResultHint( urlStr );
            q->setSaveHTTPResultHint( true );
        }
        Pipeline::instance()->resolve( q, true );

        // now we add it to the special "bookmarks" playlist, creating it if it doesn't exist. if nothing is playing, start playing the track
        QSharedPointer< LocalCollection > col = SourceList::instance()->getLocal()->collection().dynamicCast< LocalCollection >();
        playlist_ptr bookmarkpl = col->bookmarksPlaylist();
        if ( bookmarkpl.isNull() )
        {
            // create it and do the deed then
            m_waitingToBookmark = q;
            col->createBookmarksPlaylist();
            connect( col.data(), SIGNAL( bookmarkPlaylistCreated( Tomahawk::playlist_ptr ) ), this, SLOT( bookmarkPlaylistCreated( Tomahawk::playlist_ptr ) ), Qt::UniqueConnection );
        }
        else
        {
            doBookmark( bookmarkpl, q );
        }

        return true;
    }

    return false;
}


void
GlobalActionManager::shortenLinkRequestFinished()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );
    bool error = false;

    // NOTE: this should never happen
    if ( !reply )
    {
        emit shortLinkReady( QUrl( "" ), QUrl( "" ), QVariantMap() );
        return;
    }

    QVariant callbackObj;
    if ( reply->property( "callbackobj" ).isValid() )
        callbackObj = reply->property( "callbackobj" );

    // Check for the redirect attribute, as this should be the shortened link
    QVariant urlVariant = reply->attribute( QNetworkRequest::RedirectionTargetAttribute );

    // NOTE: this should never happen
    if ( urlVariant.isNull() || !urlVariant.isValid() )
        error = true;

    QUrl longUrl = reply->request().url();
    QUrl shortUrl = urlVariant.toUrl();

    // NOTE: this should never happen
    if ( !shortUrl.isValid() )
        error = true;

#ifndef ENABLE_HEADLESS
    // Success!  Here is the short link
    if ( m_clipboardLongUrl == reply->request().url() )
    {
        QClipboard* cb = QApplication::clipboard();

        QByteArray data = percentEncode( error ? longUrl : shortUrl );
        cb->setText( data );

        m_clipboardLongUrl.clear();
    }
    else
#endif
    {
        if ( !error )
            emit shortLinkReady( longUrl, shortUrl, callbackObj );
        else
            emit shortLinkReady( longUrl, longUrl, callbackObj );
    }

    reply->deleteLater();
}


#ifndef ENABLE_HEADLESS

void
GlobalActionManager::postShortenFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Q_ASSERT( reply );
    const QByteArray raw = reply->readAll();

    const QUrl url = QUrl::fromUserInput( raw );
    QClipboard* cb = QApplication::clipboard();

    const QByteArray data = percentEncode( url );
    cb->setText( data );

    reply->deleteLater();
}

#endif


void
GlobalActionManager::shortenLinkRequestError( QNetworkReply::NetworkError error )
{
    tDebug() << Q_FUNC_INFO << "Network Error:" << error;

    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );

    // NOTE: this should never happen
    if ( !reply )
    {
        emit shortLinkReady( QUrl( "" ), QUrl( "" ), QVariantMap() );
        return;
    }

    QVariantMap callbackMap;
    if ( reply->property( "callbackMap" ).canConvert< QVariantMap >() && !reply->property( "callbackMap" ).toMap().isEmpty() )
        callbackMap = reply->property( "callbackMap" ).toMap();
    reply->deleteLater();
    emit shortLinkReady( QUrl( "" ), QUrl( "" ), callbackMap );
}


void
GlobalActionManager::bookmarkPlaylistCreated( const playlist_ptr& pl )
{
    Q_ASSERT( !m_waitingToBookmark.isNull() );
    doBookmark( pl, m_waitingToBookmark );
}


void
GlobalActionManager::doBookmark( const playlist_ptr& pl, const query_ptr& q )
{
    plentry_ptr e( new PlaylistEntry );
    e->setGuid( uuid() );

    e->setDuration( q->displayQuery()->duration() );
    e->setLastmodified( 0 );
    QString annotation = "";
    if ( !q->property( "annotation" ).toString().isEmpty() )
        annotation = q->property( "annotation" ).toString();
    e->setAnnotation( annotation );
    e->setQuery( q );

    pl->createNewRevision( uuid(), pl->currentrevision(), QList< plentry_ptr >( pl->entries() ) << e );
    connect( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( showPlaylist() ) );

    m_toShow = pl;

    m_waitingToBookmark.clear();
}


#ifndef ENABLE_HEADLESS

void
GlobalActionManager::showPlaylist()
{
    if ( m_toShow.isNull() )
        return;

    ViewManager::instance()->show( m_toShow );

    m_toShow.clear();
}


void
GlobalActionManager::waitingForResolved( bool /* success */ )
{
    if ( m_waitingToPlay.data() != sender() )
    {
        m_waitingToPlay.clear();
        return;
    }

    if ( !m_waitingToPlay.isNull() && m_waitingToPlay->playable() )
    {
        // play it!
//         AudioEngine::instance()->playItem( AudioEngine::instance()->playlist(), m_waitingToPlay->results().first() );
        if ( sender() && sender()->property( "playNow" ).toBool() )
        {
            if ( !AudioEngine::instance()->playlist().isNull() )
                AudioEngine::instance()->playItem( AudioEngine::instance()->playlist(), m_waitingToPlay->results().first() );
            else
            {
                ViewManager::instance()->queue()->model()->appendQuery( m_waitingToPlay );
                AudioEngine::instance()->play();
            }
        }
        else
            AudioEngine::instance()->play();

        m_waitingToPlay.clear();
    }
}


/// SPOTIFY URL HANDLING

bool
GlobalActionManager::openSpotifyLink( const QString& link )
{
    SpotifyParser* spot = new SpotifyParser( link, this );
    connect( spot, SIGNAL( track( Tomahawk::query_ptr ) ), this, SLOT( handleOpenTrack( Tomahawk::query_ptr ) ) );

    return true;
}


bool
GlobalActionManager::openRdioLink( const QString& link )
{
    RdioParser* rdio = new RdioParser( this );
    connect( rdio, SIGNAL( track( Tomahawk::query_ptr ) ), this, SLOT( handleOpenTrack( Tomahawk::query_ptr ) ) );
    rdio->parse( link );

    return true;
}

#endif


QString
GlobalActionManager::hostname() const
{
    return QString( "http://toma.hk" );
}


QByteArray
GlobalActionManager::percentEncode( const QUrl& url ) const
{
    QByteArray data = url.toEncoded();

    data.replace( "'", "%27" ); // QUrl doesn't encode ', which it doesn't have to. Some apps don't like ' though, and want %27. Both are valid.
    data.replace( "%20", "+" );

    return data;
}
