/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright (C) 2011-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright (C) 2013, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright (C) 2013, Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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


#include "GlobalActionManager.h"

#include "accounts/AccountManager.h"
#include "accounts/spotify/SpotifyAccount.h"
#include "audio/AudioEngine.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "jobview/JobStatusView.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlaylistTemplate.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableModel.h"
#include "resolvers/ExternalResolver.h"
#include "resolvers/ScriptCommand_LookupUrl.h"
#include "utils/JspfLoader.h"
#include "utils/Logger.h"
#include "utils/SpotifyParser.h"
#include "utils/XspfLoader.h"
#include "utils/XspfGenerator.h"
#include "viewpages/SearchViewPage.h"

#include "Pipeline.h"
#include "TomahawkSettings.h"
#include "ViewManager.h"

#include <echonest/Playlist.h>

#include <QMessageBox>
#include <QFileInfo>

GlobalActionManager* GlobalActionManager::s_instance = 0;

using namespace Tomahawk;
using namespace TomahawkUtils;


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


void
GlobalActionManager::installResolverFromFile( const QString& resolverPath )
{
    const QFileInfo resolverAbsoluteFilePath( resolverPath );
    TomahawkSettings::instance()->setScriptDefaultPath( resolverAbsoluteFilePath.absolutePath() );

    if ( resolverAbsoluteFilePath.baseName() == "spotify_tomahawkresolver" )
    {
        // HACK if this is a spotify resolver, we treat it specially.
        // usually we expect the user to just download the spotify resolver from attica,
        // however developers, those who build their own tomahawk, can't do that, or linux
        // users can't do that. However, we have an already-existing SpotifyAccount that we
        // know exists that we need to use this resolver path.
        //
        // Hence, we special-case the spotify resolver and directly set the path on it here.
        Accounts::SpotifyAccount* acct = 0;
        foreach ( Accounts::Account* account, Accounts::AccountManager::instance()->accounts() )
        {
            if ( Accounts::SpotifyAccount* spotify = qobject_cast< Accounts::SpotifyAccount* >( account ) )
            {
                acct = spotify;
                break;
            }
        }

        if ( acct )
        {
            acct->setManualResolverPath( resolverPath );
            return;
        }
    }

    Accounts::Account* acct =
        Accounts::AccountManager::instance()->accountFromPath( resolverPath );

    if ( !acct )
    {
        QFileInfo fi( resolverPath );

        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage(
                                tr( "Resolver installation from file %1 failed." )
                                .arg( fi.fileName() ) ) );

        tDebug() << "Resolver was not installed:" << resolverPath;
        return;
    }

    int result = QMessageBox::question( JobStatusView::instance(),
                                        tr( "Install plug-in" ),
                                        tr( "<b>%1</b> %2<br/>"
                                            "by <b>%3</b><br/><br/>"
                                            "You are attempting to install a Tomahawk "
                                            "plug-in from an unknown source. Plug-ins from "
                                            "untrusted sources may put your data at risk.<br/>"
                                            "Do you want to install this plug-in?" )
                                        .arg( acct->accountFriendlyName() )
                                        .arg( acct->version() )
                                        .arg( acct->author() ),
                                        QMessageBox::Yes,
                                        QMessageBox::No );
    if ( result != QMessageBox::Yes )
        return;

    Accounts::AccountManager::instance()->addAccount( acct );
    TomahawkSettings::instance()->addAccount( acct->accountId() );
    Accounts::AccountManager::instance()->enableAccount( acct );
}


bool
GlobalActionManager::openUrl( const QString& url )
{
    // Native Implementations
    if ( url.startsWith( "tomahawk://" ) )
        return parseTomahawkLink( url );
    else if ( url.contains( "open.spotify.com" ) || url.startsWith( "spotify:" ) )
        return openSpotifyLink( url );

    // Can we parse the Url using a ScriptResolver?
    bool canParse = false;
    QList< QPointer< ExternalResolver > > possibleResolvers;
    foreach ( QPointer<ExternalResolver> resolver, Pipeline::instance()->scriptResolvers() )
    {
        if ( resolver->canParseUrl( url, ExternalResolver::Any ) )
        {
            canParse = true;
            possibleResolvers << resolver;
        }
    }
    if ( canParse )
    {
        m_queuedUrl = url;
        foreach ( QPointer<ExternalResolver> resolver, possibleResolvers )
        {
            ScriptCommand_LookupUrl* cmd = new ScriptCommand_LookupUrl( resolver, url );
            connect( cmd, SIGNAL( information( QString, QSharedPointer<QObject> ) ), this, SLOT( informationForUrl( QString, QSharedPointer<QObject> ) ) );
            cmd->enqueue();
        }

        return true;
    }

    return false;
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
            if ( urlHasQueryItem( u, "xspf" ) )
            {
                QUrl xspf = QUrl::fromUserInput( urlQueryItemValue( u, "xspf" ) );
                XSPFLoader* l = new XSPFLoader( true, true, this );
                tDebug() << "Loading spiff:" << xspf.toString();
                l->load( xspf );
                connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), ViewManager::instance(), SLOT( show( Tomahawk::playlist_ptr ) ) );

                return true;
            }
            else if ( urlHasQueryItem( u, "jspf" ) )
            {
                QUrl jspf = QUrl::fromUserInput( urlQueryItemValue( u, "jspf" ) );
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
        else if ( cmdType == "love" )
        {
            return handleLoveCommand( u );
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
        if ( !urlHasQueryItem( url, "xspf" ) && !urlHasQueryItem( url, "jspf" ) )
        {
            tDebug() << "No xspf or jspf to load...";
            return false;
        }
        if ( urlHasQueryItem( url, "xspf" ) )
        {
            createPlaylistFromUrl( "xspf", urlQueryItemValue( url, "xspf" ), urlHasQueryItem( url, "title" ) ? urlQueryItemValue( url, "title" ) : QString() );
            return true;
        }
        else if ( urlHasQueryItem( url, "jspf" ) )
        {
            createPlaylistFromUrl( "jspf", urlQueryItemValue( url, "jspf" ), urlHasQueryItem( url, "title" ) ? urlQueryItemValue( url, "title" ) : QString() );
            return true;
        }
    }
    else if ( parts [ 0 ] == "new" )
    {
        if ( !urlHasQueryItem( url, "title" ) )
        {
            tLog() << "New playlist command needs a title...";
            return false;
        }
        playlist_ptr pl = Playlist::create( SourceList::instance()->getLocal(), uuid(), urlQueryItemValue( url, "title" ), QString(), QString(), false );
        ViewManager::instance()->show( pl );
    }
    else if ( parts[ 0 ] == "add" )
    {
        if ( !urlHasQueryItem( url, "playlistid" ) || !urlHasQueryItem( url, "title" ) || !urlHasQueryItem( url, "artist" ) )
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
        if ( urlHasQueryItem( url, "xspf" ) )
        {
            createPlaylistFromUrl( "xspf", urlQueryItemValue( url, "xspf" ), urlHasQueryItem( url, "title" ) ? urlQueryItemValue( url, "title" ) : QString() );
            return true;
        }
        else if ( urlHasQueryItem( url, "jspf" ) )
        {
            createPlaylistFromUrl( "jspf", urlQueryItemValue( url, "jspf" ), urlHasQueryItem( url, "title" ) ? urlQueryItemValue( url, "title" ) : QString() );
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
        XSPFLoader* l= new XSPFLoader( true, true, this );
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
    return doQueueAdd( parts, urlQueryItems( url ) );
}


bool
GlobalActionManager::handleLoveCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if ( parts.isEmpty() )
    {
        tLog() << "No specific love command:" << url.toString();
        return false;
    }

    QPair< QString, QString > pair;
    QString title, artist, album;
    foreach ( pair, urlQueryItems( url ) )
    {
        if ( pair.first == "title" )
            title = pair.second;
        else if ( pair.first == "artist" )
            artist = pair.second;
        else if ( pair.first == "album" )
            album = pair.second;
    }

    track_ptr t = Track::get( artist, title, album );
    if ( t.isNull() )
        return false;

    t->setLoved( true );

    return true;
}


void
GlobalActionManager::handleOpenTrack( const query_ptr& q )
{
    ViewManager::instance()->queue()->view()->trackView()->model()->appendQuery( q );
    ViewManager::instance()->showQueuePage();

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

    ViewManager::instance()->queue()->view()->trackView()->model()->appendQueries( queries );
    ViewManager::instance()->showQueuePage();

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


void
GlobalActionManager::informationForUrl(const QString& url, const QSharedPointer<QObject>& information)
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Got Information for URL:" << url;
    if ( m_queuedUrl != url )
    {
        // This url is not anymore active, result was too late.
        return;
    }
    if ( information.isNull() )
    {
        // No information was transmitted, nothing to do.
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Empty information received.";
        return;
    }

    // If we reach this point, we found information that can be parsed.
    // So invalidate queued Url
    m_queuedUrl = "";

    // Try to interpret as Artist
    Tomahawk::artist_ptr artist = information.objectCast<Tomahawk::Artist>();
    if ( !artist.isNull() )
    {
        // The Url describes an artist
        ViewManager::instance()->show( artist );
        return;
    }

    // Try to interpret as Album
    Tomahawk::album_ptr album = information.objectCast<Tomahawk::Album>();
    if ( !album.isNull() )
    {
        // The Url describes an album
        ViewManager::instance()->show( album );
        return;
    }

    Tomahawk::playlisttemplate_ptr pltemplate = information.objectCast<Tomahawk::PlaylistTemplate>();
    if ( !pltemplate.isNull() )
    {
        ViewManager::instance()->show( pltemplate->get() );
        return;
    }

    // Try to interpret as Track/Query
    Tomahawk::query_ptr query = information.objectCast<Tomahawk::Query>();
    if ( !query.isNull() )
    {
        // The Url describes a track
        ViewManager::instance()->show( query );
        return;
    }

    // Try to interpret as Playlist
    Tomahawk::playlist_ptr playlist = information.objectCast<Tomahawk::Playlist>();
    if ( !playlist.isNull() )
    {
        // The url describes a playlist
        ViewManager::instance()->show( playlist );
        return;
    }

    // Could not cast to a known type.
    tLog() << Q_FUNC_INFO << "Can't load parsed information for " << url;
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
        doQueueAdd( parts.mid( 1 ), urlQueryItems( url ) );
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

                    QString artistText = track.host();
                    if ( artistText.isEmpty() )
                        artistText = info.absolutePath();
                    if ( artistText.isEmpty() )
                        artistText = track.toString();

                    query_ptr q = Query::get( artistText, info.baseName(), QString(), uuid(), false );

                    if ( q.isNull() )
                        continue;

                    q->setResultHint( track.toString() );
                    q->setSaveHTTPResultHint( true );

                    Pipeline::instance()->resolve( q );

                    ViewManager::instance()->queue()->view()->trackView()->model()->appendQuery( q );
                    ViewManager::instance()->showQueuePage();
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
GlobalActionManager::handleSearchCommand( const QUrl& url )
{
    // open the super collection and set this as the search filter
    QString queryStr;
    if ( urlHasQueryItem( url, "query" ) )
        queryStr = urlQueryItemValue( url, "query" );
    else
    {
        QStringList query;
        if ( urlHasQueryItem( url, "artist" ) )
            query << urlQueryItemValue( url, "artist" );
        if ( urlHasQueryItem( url, "album" ) )
            query << urlQueryItemValue( url, "album" );
        if ( urlHasQueryItem( url, "title" ) )
            query << urlQueryItemValue( url, "title" );
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
        const QString artist = urlQueryItemValue( url, "name" );
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
        const QString artist = urlQueryItemValue( url, "artist" );
        const QString album = urlQueryItemValue( url, "name" );
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
        const QString artist = urlQueryItemValue( url, "artist" );
        const QString album = urlQueryItemValue( url, "album" );
        const QString track = urlQueryItemValue( url, "track" );
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
        if ( !urlHasQueryItem( url, "title" ) || !urlHasQueryItem( url, "type" ) )
        {
            tLog() << "Station create command needs title and type..." << url.toString();
            return Tomahawk::dynplaylist_ptr();
        }
        QString title = urlQueryItemValue( url, "title" );
        QString type = urlQueryItemValue( url, "type" );
        GeneratorMode m = Static;
        if ( station )
            m = OnDemand;

        dynplaylist_ptr pl = DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), title, QString(), QString(), m, false, type );
        pl->setMode( m );
        QList< dyncontrol_ptr > controls;
        QPair< QString, QString > param;
        foreach ( param, urlQueryItems( url ) )
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

        QPair< QString, QString > pair;
        QString title, artist, album, urlStr;
        foreach ( pair, urlQueryItems( url ) )
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
    if ( !urlHasQueryItem( url, "spotifyURI" ) && !urlHasQueryItem( url, "spotifyURL" ) )
        return false;

    QString spotifyUrl = urlHasQueryItem( url, "spotifyURI" ) ? urlQueryItemValue( url, "spotifyURI" ) : urlQueryItemValue( url, "spotifyURL" );
    SpotifyParser* p = new SpotifyParser( spotifyUrl, false, this );
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
                ViewManager::instance()->queue()->view()->trackView()->model()->appendQuery( m_waitingToPlay );
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
    SpotifyParser* spot = new SpotifyParser( link, false, this );
    connect( spot, SIGNAL( track( Tomahawk::query_ptr ) ), this, SLOT( handleOpenTrack( Tomahawk::query_ptr ) ) );

    return true;
}
