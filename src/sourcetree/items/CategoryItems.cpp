/*
 *  Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *  Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "CategoryItems.h"

#include <QMimeData>

#include <echonest/Playlist.h>

#include "DropJob.h"
#include "TomahawkApp.h"
#include "ViewManager.h"
#include "ViewPage.h"
#include "SourceList.h"
#include "SourceTreeView.h"
#include "TomahawkWindow.h"
#include "utils/TomahawkUtils.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "widgets/NewPlaylistWidget.h"
#include "utils/ImageRegistry.h"
#include "utils/Logger.h"

using namespace Tomahawk;


/// CategoryAddItem

CategoryAddItem::CategoryAddItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType type )
    : SourceTreeItem( model, parent, SourcesModel::CategoryAdd )
    , m_categoryType( type )
{
}


CategoryAddItem::~CategoryAddItem()
{
}


QString
CategoryAddItem::text() const
{
    switch( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            return tr( "Create new Playlist" );

        case SourcesModel::StationsCategory:
            return tr( "Create new Station" );
    }

    return QString();
}


void
CategoryAddItem::activate()
{
    switch( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            APP->mainWindow()->createPlaylist();
            break;

        case SourcesModel::StationsCategory:
            APP->mainWindow()->createStation();
            break;
    }
}


Qt::ItemFlags
CategoryAddItem::flags() const
{
    switch ( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;

        case SourcesModel::StationsCategory:
            return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

        default:
            return Qt::ItemIsEnabled;
            break;
    }
}


QIcon
CategoryAddItem::icon() const
{
    return ImageRegistry::instance()->icon( RESPATH "images/add.svg" );
}


bool
CategoryAddItem::willAcceptDrag( const QMimeData* data ) const
{
    if ( ( m_categoryType == SourcesModel::PlaylistsCategory || m_categoryType == SourcesModel::StationsCategory ) && DropJob::acceptsMimeData( data ) )
    {
        return true;
    }

    return false;
}


SourceTreeItem::DropTypes
CategoryAddItem::supportedDropTypes( const QMimeData* data ) const
{
    SourceTreeItem::DropTypes types = DropTypesNone;

    if ( m_categoryType == SourcesModel::PlaylistsCategory )
    {
        if ( data->hasFormat( "application/tomahawk.query.list" ) )
            return types | DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
        else if ( data->hasFormat( "application/tomahawk.result.list" ) )
            return types | DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
        else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
            return types | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
        else if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
            return types | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
    }

    return types;
}


bool
CategoryAddItem::dropMimeData( const QMimeData* data, Qt::DropAction )
{
    // As DropJob always converts dropped items to query_ptrs for all tracks we need to extract album/artist metadata ourselves for stations
    if ( m_categoryType == SourcesModel::StationsCategory &&
         ( data->hasFormat( "application/tomahawk.metadata.artist" ) || data->hasFormat( "application/tomahawk.metadata.album" ) ) )
    {
        QByteArray mimeData;
        if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
            mimeData = data->data( "application/tomahawk.metadata.artist" );
        else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
            mimeData = data->data( "application/tomahawk.metadata.album" );

        QDataStream stream( &mimeData, QIODevice::ReadOnly );

        dynplaylist_ptr newpl = DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), QString(), "", SourceList::instance()->getLocal()->friendlyName(), OnDemand, false );
        newpl->setMode( OnDemand );

        QString firstArtist;
        // now we want to add each artist as a filter...
        QList< dyncontrol_ptr > contrls;
        while ( !stream.atEnd() )
        {
            QString artist;
            stream >> artist;
            if ( firstArtist.isEmpty() )
                firstArtist = artist;

            QString album;
            if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
                stream >> album; // throw away album title... we only create artists filters for now

            dyncontrol_ptr c = newpl->generator()->createControl( "Artist" );
            c->setInput( QString( "%1" ).arg( artist ) );
            contrls << c;
        }

        QString name = firstArtist.isEmpty() ? tr( "New Station" ) : tr( "%1 Station" ).arg( firstArtist );
        newpl->createNewRevision( uuid(), newpl->currentrevision(), newpl->type(), contrls );
        newpl->setProperty( "newname", name );
        connect( newpl.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( playlistToRenameLoaded() ) );

        ViewManager::instance()->show( newpl );
        return true;
    }

    // This could be needed once echonest supports filtering by album.
    // If they never will, or if they do and this code still is not used, throw it away!
    // If you enable this, make sure to remove the checks for album above.

    /* if ( m_categoryType == SourcesModel::StationsCategory && data->hasFormat( "application/tomahawk.metadata.album" ) )
    {
        QByteArray mimeData = data->data( "application/tomahawk.metadata.album" );
        QDataStream stream( &mimeData, QIODevice::ReadOnly );

        dynplaylist_ptr newpl = DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), QString(), "", SourceList::instance()->getLocal()->friendlyName(), OnDemand, false );
        newpl->setMode( OnDemand );

        QString firstAlbum;
        // now we want to add each artist as a filter...
        QList< dyncontrol_ptr > contrls;
        while ( !stream.atEnd() )
        {
            QString artist;
            stream >> artist;
            QString album;
            stream >> album;

            if ( firstAlbum.isEmpty() )
            {
                firstAlbum = album;
            }

            dyncontrol_ptr c = newpl->generator()->createControl( "Album" );
            c->setInput( QString( "%1" ).arg( artist ) );
            contrls << c;
        }

        QString name = firstAlbum.isEmpty() ? tr( "New Station" ) : tr( "%1 Station" ).arg( firstAlbum );
        newpl->rename( name );
        newpl->createNewRevision( uuid(), newpl->currentrevision(), newpl->type(), contrls );

        ViewManager::instance()->show( newpl );
        // Give a shot to try to rename it. The playlist has to be created first. ugly.
        QTimer::singleShot( 300, APP->mainWindow()->sourceTreeView(), SLOT( renamePlaylist() ) );
        return true;

    } */

    // Create a new playlist seeded with these items
    DropJob *dj = new DropJob();

    connect( dj, SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ), this, SLOT( parsedDroppedTracks( QList< Tomahawk::query_ptr > ) ), Qt::QueuedConnection );
    if ( dropType() == DropTypeAllFromArtist )
        dj->setGetWholeArtists( true );
    if ( dropType() == DropTypeThisAlbum )
        dj->setGetWholeAlbums( true );

    if ( dropType() == DropTypeLocalItems )
    {
        dj->setGetWholeArtists( true );
        dj->tracksFromMimeData( data, false, true );
    }
    else if ( dropType() == DropTypeTop50 )
    {
        dj->setGetWholeArtists( true );
        dj->tracksFromMimeData( data, false, false, true );
    }
    else
        dj->tracksFromMimeData( data, false, false );

    return true;
}


void
CategoryAddItem::playlistToRenameLoaded()
{
    Playlist* pl = qobject_cast< Playlist* >( sender() );
    Q_ASSERT( pl );

    QString name = sender()->property( "newname" ).toString();
    if ( !name.isEmpty() )
        pl->rename( name );
    else
        QTimer::singleShot( 400, APP->mainWindow()->sourceTreeView(), SLOT( renamePlaylist() ) );

    disconnect( pl, SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( playlistToRenameLoaded() ) );
    disconnect( pl, SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistToRenameLoaded() ) );
}


void
CategoryAddItem::parsedDroppedTracks( const QList< query_ptr >& tracks )
{
    if ( m_categoryType == SourcesModel::PlaylistsCategory )
    {
        playlist_ptr newpl = Playlist::create( SourceList::instance()->getLocal(), uuid(), "New Playlist", "", SourceList::instance()->getLocal()->friendlyName(), false, tracks );
        ViewManager::instance()->show( newpl );

        connect( newpl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistToRenameLoaded() ) );
    }
    else if ( m_categoryType == SourcesModel::StationsCategory )
    {
        // seed the playlist with these song or artist filters
        QString name;
        name = tracks.isEmpty() ? tr( "New Station" ) : tr( "%1 Station" ).arg( tracks.first()->track() );

        dynplaylist_ptr newpl = DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), name, "", SourceList::instance()->getLocal()->friendlyName(), OnDemand, false );
        newpl->setMode( OnDemand );

        // now we want to add each query as a song or similar artist filter...
        QList< dyncontrol_ptr > controls;
        foreach ( const Tomahawk::query_ptr& q, tracks )
        {
            dyncontrol_ptr c = newpl->generator()->createControl( "Song" );
            c->setInput( QString( "%1 %2" ).arg( q->track() ).arg( q->artist() ) );
            controls << c;
        }

        newpl->createNewRevision( uuid(), newpl->currentrevision(), newpl->type(), controls );

        ViewManager::instance()->show( newpl );
        connect( newpl.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( playlistToRenameLoaded() ) );
    }
}


int
CategoryAddItem::peerSortValue() const
{
    return INT_MAX; // after any siblings
}


/// CategoryItem

CategoryItem::CategoryItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType category, bool showAddItem )
    : SourceTreeItem( model, parent, SourcesModel::Category )
    , m_category( category )
    , m_addItem( 0 )
    , m_showAdd( showAddItem )
{
    // in the constructor we're still being added to the parent, so we don't exist to have rows addded yet. so this is safe.
    //     beginRowsAdded( 0, 0 );
    if ( m_showAdd )
    {
        m_addItem = new CategoryAddItem( model, this, m_category );
    }
    //     endRowsAdded();
}


void
CategoryItem::insertItem( SourceTreeItem* item )
{
    insertItems( QList< SourceTreeItem* >() << item );
}


void
CategoryItem::insertItems( QList< SourceTreeItem* > items )
{
    // add the items to the category, and connect to the signals
    int curCount = children().size();
    if ( m_showAdd ) // if there's an add item, add it before that
        curCount--;

    beginRowsAdded( curCount, curCount + items.size() - 1 );
    foreach( SourceTreeItem* item, items )
    {
        insertChild( children().count() - 1, item );
    }
    endRowsAdded();
}


int
CategoryItem::peerSortValue() const
{
    if ( m_category == SourcesModel::PlaylistsCategory )
        return -100;
    else if ( m_category == SourcesModel::StationsCategory )
        return 100;
    else
        return 0;
}


void
CategoryItem::activate()
{
    emit toggleExpandRequest( this );
}


QString
CategoryItem::text() const
{
    switch( m_category )
    {
    case SourcesModel::PlaylistsCategory:
        return tr( "Playlists" );
    case SourcesModel::StationsCategory:
        return tr( "Stations" );
    }
    return QString();
}


Qt::ItemFlags
CategoryItem::flags() const
{
    return Qt::ItemIsEnabled;
}


SourcesModel::CategoryType
CategoryItem::categoryType()
{
    return m_category;
}
