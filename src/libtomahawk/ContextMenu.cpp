/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ContextMenu.h"

#include "audio/AudioEngine.h"
#include "playlist/PlaylistView.h"
#include "filemetadata/MetadataEditor.h"
#include "GlobalActionManager.h"
#include "ViewManager.h"
#include "Query.h"
#include "Result.h"
#include "collection/Collection.h"
#include "Source.h"
#include "SourceList.h"
#include "Artist.h"
#include "Album.h"

#include "utils/ImageRegistry.h"
#include "utils/Logger.h"

using namespace Tomahawk;


ContextMenu::ContextMenu( QWidget* parent )
    : QMenu( parent )
    , m_loveAction( 0 )
{
    m_sigmap = new QSignalMapper( this );
    connect( m_sigmap, SIGNAL( mapped( int ) ), SLOT( onTriggered( int ) ) );

    m_supportedActions = ActionPlay | ActionQueue | ActionPlaylist | ActionCopyLink | ActionLove | ActionStopAfter | ActionPage | ActionEditMetadata;
}


ContextMenu::~ContextMenu()
{
}


void
ContextMenu::clear()
{
    QMenu::clear();

    m_queries.clear();
    m_albums.clear();
    m_artists.clear();
}


unsigned int
ContextMenu::itemCount() const
{
   return m_queries.count() + m_artists.count() + m_albums.count();
}


void
ContextMenu::addToPlaylist( int playlistIdx )
{
    Tomahawk::playlist_ptr playlist = m_playlists.at( playlistIdx );
    playlist->addEntries( m_queries, playlist->currentrevision() );
}

bool caseInsensitiveLessThan(Tomahawk::playlist_ptr &s1, Tomahawk::playlist_ptr &s2)
{
    return s1->title().toLower() < s2->title().toLower();
}

void
ContextMenu::setQueries( const QList<Tomahawk::query_ptr>& queries )
{
    if ( queries.isEmpty() )
        return;

    QMenu::clear();
    m_queries.clear();
    m_queries << queries;

    if ( m_supportedActions & ActionPlay && itemCount() == 1 )
        m_sigmap->setMapping( addAction( tr( "&Play" ) ), ActionPlay );

    if ( m_supportedActions & ActionQueue )
        m_sigmap->setMapping( addAction( tr( "Add to &Queue" ) ), ActionQueue );

    if ( m_supportedActions & ActionPlaylist ) {
        // Get the current list of all playlists.
        m_playlists = QList< Tomahawk::playlist_ptr >( SourceList::instance()->getLocal()->dbCollection()->playlists() );
        // Sort the playlist
        qSort( m_playlists.begin(), m_playlists.end(), caseInsensitiveLessThan );
        m_playlists_sigmap = new QSignalMapper( this );

        // Build the menu listing all available playlists
        QMenu* playlistMenu = addMenu( tr( "Add to &Playlist" ) );
        for ( int i = 0; i < m_playlists.length(); ++i )
        {
            QAction* action = new QAction( m_playlists.at(i)->title() , this );
            playlistMenu->addAction(action);
            m_playlists_sigmap->setMapping( action, i );
            connect( action, SIGNAL( triggered() ), m_playlists_sigmap, SLOT( map() ));
        }
        connect( m_playlists_sigmap, SIGNAL( mapped( int ) ), this, SLOT( addToPlaylist( int ) ) );
    }

    if ( m_supportedActions & ActionStopAfter && itemCount() == 1 )
    {
        if ( AudioEngine::instance()->stopAfterTrack() == queries.first() )
            m_sigmap->setMapping( addAction( tr( "Continue Playback after this &Track" ) ), ActionStopAfter );
        else
            m_sigmap->setMapping( addAction( tr( "Stop Playback after this &Track" ) ), ActionStopAfter );
    }

    addSeparator();

    if ( m_supportedActions & ActionLove && itemCount() == 1 )
    {
        m_loveAction = addAction( tr( "&Love" ) );
        m_sigmap->setMapping( m_loveAction, ActionLove );

        connect( queries.first().data(), SIGNAL( socialActionsLoaded() ), SLOT( onSocialActionsLoaded() ) );
        onSocialActionsLoaded();
    }

    addSeparator();

    if ( m_supportedActions & ActionPage && itemCount() == 1 )
    {
        // Ampersands need to be escaped as they indicate a keyboard shortcut
        const QString track = m_queries.first()->track().replace( QString( "&" ), QString( "&&" ) );
        m_sigmap->setMapping( addAction( ImageRegistry::instance()->icon( RESPATH "images/track-icon.svg" ),
                                         tr( "&Go to \"%1\"" ).arg( track ) ), ActionTrackPage );
        if ( !m_queries.first()->album().isEmpty() ) {
            const QString album = m_queries.first()->album().replace( QString( "&" ), QString( "&&" ) );
            m_sigmap->setMapping( addAction( ImageRegistry::instance()->icon( RESPATH "images/album-icon.svg" ),
                                             tr( "Go to \"%1\"" ).arg( album ) ), ActionAlbumPage );
        }
        const QString artist = m_queries.first()->artist().replace( QString( "&" ), QString( "&&" ) );
        m_sigmap->setMapping( addAction( ImageRegistry::instance()->icon( RESPATH "images/artist-icon.svg" ),
                                         tr( "Go to \"%1\"" ).arg( artist ) ), ActionArtistPage );
    }

    addSeparator();

    if ( m_supportedActions & ActionCopyLink && itemCount() == 1 )
        m_sigmap->setMapping( addAction( tr( "&Copy Track Link" ) ), ActionCopyLink );

    if ( m_supportedActions & ActionEditMetadata && itemCount() == 1 )
        m_sigmap->setMapping( addAction( tr( "Properties..." ) ), ActionEditMetadata );

    addSeparator();

    if ( m_supportedActions & ActionDelete )
        m_sigmap->setMapping( addAction( queries.count() > 1 ? tr( "&Delete Items" ) : tr( "&Delete Item" ) ), ActionDelete );

    foreach ( QAction* action, actions() )
    {
        connect( action, SIGNAL( triggered() ), m_sigmap, SLOT( map() ) );
    }
}


void
ContextMenu::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    QList<query_ptr> queries;
    queries << query;
    setQueries( queries );
}


void
ContextMenu::setAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    if ( albums.isEmpty() )
        return;

    QMenu::clear();
    m_albums.clear();
    m_albums << albums;

    if ( m_supportedActions & ActionQueue )
        m_sigmap->setMapping( addAction( tr( "Add to &Queue" ) ), ActionQueue );

    addSeparator();

    if ( m_supportedActions & ActionPage && itemCount() == 1 )
    {
        const QString album = m_albums.first()->name().replace( QString( "&" ), QString( "&&" ) );
        m_sigmap->setMapping( addAction( ImageRegistry::instance()->icon( RESPATH "images/album-icon.svg" ),
                                         tr( "&Go to \"%1\"" ).arg( album ) ), ActionAlbumPage );
        const QString artist = m_albums.first()->artist()->name().replace( QString( "&" ), QString( "&&" ) );
        m_sigmap->setMapping( addAction( ImageRegistry::instance()->icon( RESPATH "images/artist-icon.svg" ),
                                         tr( "Go to \"%1\"" ).arg( artist ) ), ActionArtistPage );
    }

    //m_sigmap->setMapping( addAction( tr( "&Add to Playlist" ) ), ActionAddToPlaylist );

    addSeparator();

    if ( m_supportedActions & ActionCopyLink && itemCount() == 1 )
        m_sigmap->setMapping( addAction( tr( "Copy Album &Link" ) ), ActionCopyLink );

    foreach ( QAction* action, actions() )
    {
        connect( action, SIGNAL( triggered() ), m_sigmap, SLOT( map() ) );
    }
}


void
ContextMenu::setAlbum( const Tomahawk::album_ptr& album )
{
    QList<album_ptr> albums;
    albums << album;
    setAlbums( albums );
}


void
ContextMenu::setArtists( const QList<Tomahawk::artist_ptr>& artists )
{
    if ( artists.isEmpty() )
        return;

    QMenu::clear();
    m_artists.clear();
    m_artists << artists;

/*    if ( m_supportedActions & ActionPlay && itemCount() == 1 )
        m_sigmap->setMapping( addAction( tr( "Show &Artist Page" ) ), ActionPlay );*/

    if ( m_supportedActions & ActionQueue )
        m_sigmap->setMapping( addAction( tr( "Add to &Queue" ) ), ActionQueue );

    addSeparator();

    if ( m_supportedActions & ActionPage && itemCount() == 1 ) {
        const QString artist = m_artists.first()->name().replace( QString( "&" ), QString( "&&" ) );
        m_sigmap->setMapping( addAction( ImageRegistry::instance()->icon( RESPATH "images/artist-icon.svg" ),
                                         tr( "&Go to \"%1\"" ).arg( artist ) ), ActionArtistPage );
    }

    //m_sigmap->setMapping( addAction( tr( "&Add to Playlist" ) ), ActionAddToPlaylist );

    addSeparator();

    if ( m_supportedActions & ActionCopyLink && itemCount() == 1 )
        m_sigmap->setMapping( addAction( tr( "Copy Artist &Link" ) ), ActionCopyLink );

    foreach ( QAction* action, actions() )
    {
        connect( action, SIGNAL( triggered() ), m_sigmap, SLOT( map() ) );
    }
}


void
ContextMenu::setArtist( const Tomahawk::artist_ptr& artist )
{
    QList<artist_ptr> artists;
    artists << artist;
    setArtists( artists );
}


void
ContextMenu::onTriggered( int action )
{
    switch ( action )
    {
        case ActionQueue:
            addToQueue();
            break;

        case ActionCopyLink:
            copyLink();
            break;

        case ActionTrackPage:
        case ActionArtistPage:
        case ActionAlbumPage:
            openPage( (MenuActions)action );
            break;

        case ActionLove:
            m_queries.first()->setLoved( !m_queries.first()->loved() );
            break;

        case ActionStopAfter:
            if ( m_queries.first()->equals( AudioEngine::instance()->stopAfterTrack() ) )
                AudioEngine::instance()->setStopAfterTrack( query_ptr() );
            else
                AudioEngine::instance()->setStopAfterTrack( m_queries.first() );
            break;

        case ActionEditMetadata:
            {
                MetadataEditor* d = new MetadataEditor( m_queries.first(), m_interface, this );
                d->show();
            }
            break;

        default:
            emit triggered( action );
    }
}



void
ContextMenu::addToQueue()
{
    foreach ( const query_ptr& query, m_queries )
    {
        ViewManager::instance()->queue()->model()->appendQuery( query );
    }
    foreach ( const artist_ptr& artist, m_artists )
    {
        ViewManager::instance()->queue()->model()->appendArtist( artist );
    }
    foreach ( const album_ptr& album, m_albums )
    {
        ViewManager::instance()->queue()->model()->appendAlbum( album );
    }

    ViewManager::instance()->showQueue();
}


void
ContextMenu::copyLink()
{
    if ( m_queries.count() )
    {
        GlobalActionManager::instance()->copyToClipboard( m_queries.first() );
    }
    else if ( m_albums.count() )
    {
        GlobalActionManager::instance()->copyOpenLink( m_albums.first() );
    }
    else if ( m_artists.count() )
    {
        GlobalActionManager::instance()->copyOpenLink( m_artists.first() );
    }
}


void
ContextMenu::openPage( MenuActions action )
{
    if ( m_queries.count() )
    {
        if ( action == ActionTrackPage )
        {
            ViewManager::instance()->show( m_queries.first() );
        }
        else
        {
            const Tomahawk::artist_ptr artist = Artist::get( m_queries.first()->artist(), false );
            if ( action == ActionArtistPage )
            {
                ViewManager::instance()->show( artist );
            }
            else if ( action == ActionAlbumPage )
            {
                ViewManager::instance()->show( Album::get( artist, m_queries.first()->album(), false ) );
            }
        }
    }
    else if ( m_albums.count() )
    {
        if ( action == ActionArtistPage )
        {
            ViewManager::instance()->show( m_albums.first()->artist() );
        }
        else
        {
            ViewManager::instance()->show( m_albums.first() );
        }
    }
    else if ( m_artists.count() )
    {
        ViewManager::instance()->show( m_artists.first() );
    }
}


void
ContextMenu::onSocialActionsLoaded()
{
    if ( m_queries.isEmpty() || m_queries.first().isNull() )
        return;

    if ( m_loveAction && m_queries.first()->loved() )
    {
        m_loveAction->setText( tr( "Un-&Love" ) );
        m_loveAction->setIcon( ImageRegistry::instance()->icon( RESPATH "images/not-loved.svg" ) );
    }
    else if ( m_loveAction )
    {
        m_loveAction->setText( tr( "&Love" ) );
        m_loveAction->setIcon( ImageRegistry::instance()->icon( RESPATH "images/loved.svg" ) );
    }
}


void
ContextMenu::setPlaylistInterface( const Tomahawk::playlistinterface_ptr& plInterface )
{
    m_interface = plInterface;
}
