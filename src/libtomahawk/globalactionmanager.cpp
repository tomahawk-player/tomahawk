/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

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

#include "globalactionmanager.h"

#include "audio/audioengine.h"
#include "utils/xspfloader.h"
#include "sourcelist.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "viewmanager.h"
#include "playlist/topbar/topbar.h"
#include "pipeline.h"
#include "database/localcollection.h"
#include "playlist/playlistview.h"
#include "echonest/Playlist.h"

#include <QUrl>
#include <Playlist.h>
#include <qclipboard.h>
#include <qapplication.h>

GlobalActionManager* GlobalActionManager::s_instance = 0;

GlobalActionManager*
GlobalActionManager::instance()
{
    if( !s_instance )
        s_instance = new GlobalActionManager;

    return s_instance;
}


GlobalActionManager::GlobalActionManager( QObject* parent )
    : QObject( parent )
{

}

GlobalActionManager::~GlobalActionManager()
{}

QUrl
GlobalActionManager::openLinkFromQuery( const Tomahawk::query_ptr& query ) const
{
    QUrl link( "tomahawk://open/track/" );
    if( !query->track().isEmpty() )
        link.addQueryItem( "title", query->track() );
    if( !query->artist().isEmpty() )
        link.addQueryItem( "artist", query->artist() );
    if( !query->album().isEmpty() )
        link.addQueryItem( "album", query->album() );

    return link;
}

void
GlobalActionManager::copyToClipboard( const Tomahawk::query_ptr& query ) const
{
    QClipboard* cb = QApplication::clipboard();
    cb->setText( openLinkFromQuery( query ).toEncoded() );
}


bool
GlobalActionManager::parseTomahawkLink( const QString& url )
{
    if( url.contains( "tomahawk://" ) ) {
        QString cmd = url.mid( 11 );
        qDebug() << "Parsing tomahawk link command" << cmd;

        QString cmdType = cmd.split( "/" ).first();
        QUrl u( cmd );

        // for backwards compatibility
        if( cmdType == "load" ) {
            if( u.hasQueryItem( "xspf" ) ) {
                QUrl xspf = QUrl::fromUserInput( u.queryItemValue( "xspf" ) );
                XSPFLoader* l = new XSPFLoader( true, this );
                qDebug() << "Loading spiff:" << xspf.toString();
                l->load( xspf );
                connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), ViewManager::instance(), SLOT( show( Tomahawk::playlist_ptr ) ) );

                return true;
            }
        }

        if( cmdType == "playlist" ) {
            return handlePlaylistCommand( u );
        } else if( cmdType == "collection" ) {
            return handleCollectionCommand( u );
        } else if( cmdType == "queue" ) {
            return handleQueueCommand( u );
        } else if( cmdType == "station" ) {
            return handleStationCommand( u );
        } else if( cmdType == "search" ) {
            return handleSearchCommand( u );
        } else if( cmdType == "play" ) {
            return handlePlayCommand( u );
        } else if( cmdType == "bookmark" ) {
            return handlePlayCommand( u );
        } else if( cmdType == "open" ) {
            return handleOpenCommand( u );
        } else {
            qDebug() << "Tomahawk link not supported, command not known!" << cmdType << u.path();
            return false;
        }
    } else {
        qDebug() << "Not a tomahawk:// link!";
        return false;
    }
}

bool
GlobalActionManager::handlePlaylistCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if( parts.isEmpty() ) {
        qDebug() << "No specific playlist command:" << url.toString();
        return false;
    }

    if( parts[ 0 ] == "import" ) {
        if( !url.hasQueryItem( "xspf" ) ) {
            qDebug() << "No xspf to load...";
            return false;
        }
        QUrl xspf = QUrl( url.queryItemValue( "xspf" ) );
        QString title =  url.hasQueryItem( "title" ) ? url.queryItemValue( "title" ) : QString();
        XSPFLoader* l= new XSPFLoader( true, this );
        l->setOverrideTitle( title );
        l->load( xspf );
        connect( l, SIGNAL( ok( Tomahawk::playlist_ptr ) ), ViewManager::instance(), SLOT( show( Tomahawk::playlist_ptr ) ) );

    } else if( parts [ 0 ] == "new" ) {
        if( !url.hasQueryItem( "title" ) ) {
            qDebug() << "New playlist command needs a title...";
            return false;
        }
        Tomahawk::playlist_ptr pl = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), uuid(), url.queryItemValue( "title" ), QString(), QString(), false );
        pl->createNewRevision( uuid(), pl->currentrevision(), QList< Tomahawk::plentry_ptr >() );
        ViewManager::instance()->show( pl );
    } else if( parts[ 0 ] == "add" ) {
        if( !url.hasQueryItem( "playlistid" ) || !url.hasQueryItem( "title" ) || !url.hasQueryItem( "artist" ) ) {
            qDebug() << "Add to playlist command needs playlistid, track, and artist..." << url.toString();
            return false;
        }
        // TODO implement. Let the user select what playlist to add to
        return false;
    }

    return false;
}

bool
GlobalActionManager::handleCollectionCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if( parts.isEmpty() ) {
        qDebug() << "No specific collection command:" << url.toString();
        return false;
    }

    if( parts[ 0 ] == "add" ) {
        // TODO implement
    }

    return false;
}

bool
GlobalActionManager::handleOpenCommand(const QUrl& url)
{
    QStringList parts = url.path().split( "/" ).mid( 1 );
    if( parts.isEmpty() ) {
        qDebug() << "No specific type to open:" << url.toString();
        return false;
    }
    // TODO user configurable in the UI
    return doQueueAdd( parts, url.queryItems() );
}


bool
GlobalActionManager::handleQueueCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if( parts.isEmpty() ) {
        qDebug() << "No specific queue command:" << url.toString();
        return false;
    }

    if( parts[ 0 ] == "add" ) {
        doQueueAdd( parts.mid( 1 ), url.queryItems() );
    } else {
        qDebug() << "Only queue/add/track is support at the moment, got:" << parts;
        return false;
    }

    return false;
}

bool
GlobalActionManager::doQueueAdd( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems )
{
    if( parts.size() && parts[ 0 ] == "track" ) {
        QPair< QString, QString > pair;

        QString title, artist, album, urlStr;
        foreach( pair, queryItems ) {
            if( pair.first == "title" )
                title = pair.second;
            else if( pair.first == "artist" )
                artist = pair.second;
            else if( pair.first == "album" )
                album = pair.second;
            else if( pair.first == "url" )
                urlStr = pair.second;
        }

        if( !title.isEmpty() || !artist.isEmpty() || !album.isEmpty() ) { // an individual; query to add to queue
            Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album );
            if( !urlStr.isEmpty() )
                q->setResultHint( urlStr );
            Tomahawk::Pipeline::instance()->resolve( q, true );

            ViewManager::instance()->queue()->model()->append( q );
            ViewManager::instance()->showQueue();

            if( !AudioEngine::instance()->isPlaying() ) {
                connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( waitingForResolved( bool ) ) );
                m_waitingToPlay = q;
            }
            return true;

        } else { // a list of urls to add to the queue
            foreach( pair, queryItems ) {
                if( pair.first != "url" )
                    continue;
                QUrl track = QUrl::fromUserInput( pair.second  );
                //FIXME: isLocalFile is Qt 4.8
                if( track.toString().startsWith( "file://" ) ) { // it's local, so we see if it's in the DB and load it if so
                    // TODO
                } else { // give it a web result hint
                    QFileInfo info( track.path() );
                    Tomahawk::query_ptr q = Tomahawk::Query::get( QString(), info.baseName(), QString() );
                    q->setResultHint( track.toString() );
                    Tomahawk::Pipeline::instance()->resolve( q, true );

                    ViewManager::instance()->queue()->model()->append( q );
                    ViewManager::instance()->showQueue();
                }
                return true;
            }
        }
    }
    return false;
}

bool
GlobalActionManager::handleSearchCommand( const QUrl& url )
{
    // open the super collection and set this as the search filter
    QStringList query;
    if( url.hasQueryItem( "artist" ) )
        query << url.queryItemValue( "artist" );
    if( url.hasQueryItem( "album" ) )
        query << url.queryItemValue( "album" );
    if( url.hasQueryItem( "title" ) )
        query << url.queryItemValue( "title" );
    QString queryStr = query.join( " " );

    if( queryStr.isEmpty() )
        return false;

    ViewManager::instance()->showSuperCollection();
    ViewManager::instance()->topbar()->setFilter( queryStr );
    return true;
}

bool
GlobalActionManager::handleStationCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if( parts.isEmpty() ) {
        qDebug() << "No specific station command:" << url.toString();
        return false;
    }

    if( parts[ 0 ] == "create" ) {
        if( !url.hasQueryItem( "title" ) || !url.hasQueryItem( "type" ) ) {
            qDebug() << "Station create command needs title and type..." << url.toString();
            return false;
        }
        QString title = url.queryItemValue( "title" );
        QString type = url.queryItemValue( "type" );
        Tomahawk::dynplaylist_ptr pl = Tomahawk::DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), title, QString(), QString(), Tomahawk::OnDemand, false, type );
        QList< Tomahawk::dyncontrol_ptr > controls;
        QPair< QString, QString > param;
        foreach( param, url.queryItems() ) {
            if( param.first == "artist" ) {
                Tomahawk::dyncontrol_ptr c = pl->generator()->createControl( "Artist" );
                c->setInput( param.second );
                c->setMatch( QString::number( (int)Echonest::DynamicPlaylist::ArtistRadioType ) );
                controls << c;
            } /*else if( param.first == "hotttnesss" ) { TODO
                Tomahawk::dyncontrol_ptr c = pl->generator()->createControl( "Artist" );
                c->setInput( param.second );
                c->setMatch( 0 );
                controls << c;
            } */
        }
        pl->createNewRevision( uuid(), pl->currentrevision(), type, controls );
        return true;
    }

    return false;
}

bool
GlobalActionManager::handlePlayCommand( const QUrl& url )
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if( parts.isEmpty() ) {
        qDebug() << "No specific play command:" << url.toString();
        return false;
    }

    if( parts[ 0 ] == "track" ) {
        QPair< QString, QString > pair;
        QString title, artist, album, urlStr;
        foreach( pair, url.queryItems() ) {
            if( pair.first == "title" )
                title = pair.second;
            else if( pair.first == "artist" )
                artist = pair.second;
            else if( pair.first == "album" )
                album = pair.second;
            else if( pair.first == "url" )
                urlStr = pair.second;
        }
        Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album );
        if( !urlStr.isEmpty() )
            q->setResultHint( urlStr );
        Tomahawk::Pipeline::instance()->resolve( q, true );

        m_waitingToPlay = q;
        connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( waitingForResolved( bool ) ) );

        return true;
    }

    return false;
}

bool GlobalActionManager::handleBookmarkCommand(const QUrl& url)
{
    QStringList parts = url.path().split( "/" ).mid( 1 ); // get the rest of the command
    if( parts.isEmpty() ) {
        qDebug() << "No specific bookmark command:" << url.toString();
        return false;
    }

    if( parts[ 0 ] == "track" ) {
        QPair< QString, QString > pair;
        QString title, artist, album, urlStr;
        foreach( pair, url.queryItems() ) {
            if( pair.first == "title" )
                title = pair.second;
            else if( pair.first == "artist" )
                artist = pair.second;
            else if( pair.first == "album" )
                album = pair.second;
            else if( pair.first == "url" )
                urlStr = pair.second;
        }
        Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album );
        if( !urlStr.isEmpty() )
            q->setResultHint( urlStr );
        Tomahawk::Pipeline::instance()->resolve( q, true );

        // now we add it to the special "bookmarks" playlist, creating it if it doesn't exist. if nothing is playing, start playing the track
        QSharedPointer< LocalCollection > col = SourceList::instance()->getLocal()->collection().dynamicCast< LocalCollection >();
        Tomahawk::playlist_ptr bookmarkpl = col->bookmarksPlaylist();
        if( bookmarkpl.isNull() ) { // create it and do the deed then
            m_waitingToBookmark = q;
            col->createBookmarksPlaylist();
            connect( col.data(), SIGNAL( bookmarkPlaylistCreated( Tomahawk::playlist_ptr ) ), this, SLOT( bookmarkPlaylistCreated( Tomahawk::playlist_ptr ) ), Qt::UniqueConnection );
        } else {
            doBookmark( bookmarkpl, q );
        }

        return true;
    }

    return false;
}



void
GlobalActionManager::bookmarkPlaylistCreated( const Tomahawk::playlist_ptr& pl )
{
    Q_ASSERT( !m_waitingToBookmark.isNull() );
    doBookmark( pl, m_waitingToBookmark );
}

void
GlobalActionManager::doBookmark( const Tomahawk::playlist_ptr& pl, const Tomahawk::query_ptr& q )
{
    Tomahawk::plentry_ptr e( new Tomahawk::PlaylistEntry );
    e->setGuid( uuid() );

    if ( q->results().count() )
        e->setDuration( q->results().at( 0 )->duration() );
    else
        e->setDuration( 0 );

    e->setLastmodified( 0 );
    e->setAnnotation( "" ); // FIXME
    e->setQuery( q );

    pl->createNewRevision( uuid(), pl->currentrevision(), QList< Tomahawk::plentry_ptr >( pl->entries() ) << e );
    connect( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( showPlaylist() ) );

    m_toShow = pl;

    m_waitingToBookmark.clear();
}

void
GlobalActionManager::showPlaylist()
{
    if( m_toShow.isNull() )
        return;

    ViewManager::instance()->show( m_toShow );

    m_toShow.clear();
}

void
GlobalActionManager::waitingForResolved( bool success )
{
    if( success && !m_waitingToPlay.isNull() && !m_waitingToPlay->results().isEmpty() ) { // play it!
//         AudioEngine::instance()->playItem( AudioEngine::instance()->playlist(), m_waitingToPlay->results().first() );
        AudioEngine::instance()->play();
    }

    m_waitingToPlay.clear();
}


