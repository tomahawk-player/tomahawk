/*
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


#include "sourcetreeitem.h"

#include "playlist.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include <source.h>
#include <playlist/playlistmanager.h>

using namespace Tomahawk;

SourceTreeItem::SourceTreeItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::RowType thisType )
    : QObject()
    , m_type( thisType )
    , m_parent( parent )
    , m_model( model )
{
    connect( this, SIGNAL( beginChildRowsAdded( int,int ) ), m_model, SLOT( onItemRowsAddedBegin( int,int ) ) );
    connect( this, SIGNAL( beginChildRowsRemoved( int,int ) ), m_model, SLOT( onItemRowsRemovedBegin( int,int ) ) );
    connect( this, SIGNAL( childRowsAdded() ), m_model, SLOT( onItemRowsAddedDone() ) );
    connect( this, SIGNAL( childRowsRemoved() ), m_model, SLOT( onItemRowsRemovedDone() ) );
}


SourceTreeItem::~SourceTreeItem()
{
    qDeleteAll( m_children );
}


/// Category item

void 
CategoryItem::activate()
{
    if( m_category == SourcesModel::StationsCategory ) {
        // TODO activate stations page
    }
}


/// PlaylistItem

PlaylistItem::PlaylistItem( SourcesModel* mdl, SourceTreeItem* parent, const playlist_ptr& pl )
    : SourceTreeItem( mdl, parent, SourcesModel::StaticPlaylist )
    , m_loaded( false )
    , m_playlist( pl )
{
    connect( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
              SLOT( onPlaylistLoaded( Tomahawk::PlaylistRevision ) ), Qt::QueuedConnection );
    connect( pl.data(), SIGNAL( changed() ),
              SIGNAL( updated() ), Qt::QueuedConnection );
}


QString 
PlaylistItem::text() const
{
    return m_playlist->title();
}

Tomahawk::playlist_ptr 
PlaylistItem::playlist() const
{
    return m_playlist;
}
void 
PlaylistItem::onPlaylistLoaded( Tomahawk::PlaylistRevision revision )
{
    m_loaded = true;
    emit updated();
}

void 
PlaylistItem::onPlaylistChanged()
{
    emit updated();
}


Qt::ItemFlags 
PlaylistItem::flags()
{
    Qt::ItemFlags flags = SourceTreeItem::flags();
    
    if( !m_loaded )
        flags &= !Qt::ItemIsEnabled;
    
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    if( playlist()->author()->isLocal() )
        flags |= Qt::ItemIsEditable;
    
    return flags;
}

void 
PlaylistItem::activate()
{
    PlaylistManager::instance()->show( m_playlist );
}

void 
PlaylistItem::setLoaded( bool loaded )
{
    m_loaded = loaded;
}


/// Dynamic Playlist Item
/*
DynPlaylistItem::DynPlaylistItem( SourcesModel* mdl, SourceTreeItem* parent, const dynplaylist_ptr& pl )
    : PlaylistItem( mdl, parent, pl.staticCast< Playlist >() )
{
    setLoaded( false );
    connect( pl.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision) ),
               SLOT( onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision ) ), Qt::QueuedConnection );
    connect( pl.data(), SIGNAL( changed() ),
               SIGNAL( updated() ), Qt::QueuedConnection );
}


QString 
DynPlaylistItem::text()
{
    return m_dynplaylist->title();
}

Tomahawk::playlist_ptr 
DynPlaylistItem::playlist() const
{
    return m_dynplaylist.staticCast<Tomahawk::Playlist>();
}

// Tomahawk::dynplaylist_ptr 
// DynPlaylistItem::playlist() const
// {
//     return m_dynplaylist;
// }

void 
DynPlaylistItem::activate()
{
    PlaylistManager::instance()->show( m_playlist );
}


void 
DynPlaylistItem::onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision revision )
{
    setLoaded( true );
}*/


/// CollectionItem

CollectionItem::CollectionItem(  SourcesModel* mdl, SourceTreeItem* parent, const Tomahawk::source_ptr& source ) 
    : SourceTreeItem( mdl, parent, SourcesModel::Collection )
    , m_source( source )
{
    // create category item
    m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory );
    appendChild( m_playlists );
    
    m_stations = new CategoryItem( model(), this, SourcesModel::StationsCategory );
    appendChild( m_stations );
    
    // ugh :( we're being added by the model, no need to notify for added rows now
    m_playlists->blockSignals( true );
    onPlaylistsAdded( source->collection()->playlists() );
    m_playlists->blockSignals( false );
    
    connect( source->collection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsAdded( QList<Tomahawk::playlist_ptr> ) ) );
    connect( source->collection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsDeleted( QList<Tomahawk::playlist_ptr> ) ) );
}

Tomahawk::source_ptr 
CollectionItem::source() const
{
    return m_source;
}

QString
CollectionItem::text() const
{
    return m_source.isNull() ? "Super Collection" : m_source->friendlyName();
}

void 
CollectionItem::activate()
{
    if( source().isNull() ) {
        PlaylistManager::instance()->showSuperCollection();
    } else {
        PlaylistManager::instance()->show( source()->collection() );
    }
}


void 
CollectionItem::onPlaylistsAdded( const QList< playlist_ptr >& playlists )
{
    if( playlists.isEmpty() )
        return;
    
    // add the items to the category, and connect to the signals
    int curCount = m_playlists->children().size();
    // TODO add items to one-minus the end of the category. this leaves room for the  [ + New Playlist ] item
    m_playlists->beginRowsAdded( curCount, curCount + playlists.size() - 1 );
    
    foreach( const playlist_ptr& p, playlists )
    {   
        PlaylistItem* plItem = new PlaylistItem( model(), m_playlists, p );
        m_playlists->appendChild( plItem );
        qDebug() << "Playlist added:" << p->title() << p->creator() << p->info();
        p->loadRevision();
    }
    m_playlists->endRowsAdded();
}

void 
CollectionItem::onPlaylistsDeleted( const QList< playlist_ptr >& playlists )
{
    int curCount = m_playlists->children().count();
    foreach( const playlist_ptr& playlist, playlists ) {
        for( int i = 0; i < curCount; i++ ) {
            PlaylistItem* pl = qobject_cast< PlaylistItem* >( m_playlists->children().at( i ) );
            if( pl && pl->playlist() == playlist ) {
                m_playlists->beginRowsRemoved( i, i );
                m_playlists->children().removeAt( i );
                m_playlists->endRowsRemoved();
            }
        }
    }
}

