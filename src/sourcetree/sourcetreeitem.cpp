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
#include "source.h"
#include "viewmanager.h"
#include "tomahawk/tomahawkapp.h"
#include "widgets/newplaylistwidget.h"

#include <QMimeData>

using namespace Tomahawk;

SourceTreeItem::SourceTreeItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::RowType thisType, int index )
    : QObject()
    , m_type( thisType )
    , m_parent( parent )
    , m_model( model )
{
    connect( this, SIGNAL( beginChildRowsAdded( int,int ) ), m_model, SLOT( onItemRowsAddedBegin( int,int ) ) );
    connect( this, SIGNAL( beginChildRowsRemoved( int,int ) ), m_model, SLOT( onItemRowsRemovedBegin( int,int ) ) );
    connect( this, SIGNAL( childRowsAdded() ), m_model, SLOT( onItemRowsAddedDone() ) );
    connect( this, SIGNAL( childRowsRemoved() ), m_model, SLOT( onItemRowsRemovedDone() ) );
    connect( this, SIGNAL( updated() ), m_model, SLOT( itemUpdated() ) );

    if( !m_parent )
        return;

    // caller must call begin/endInsertRows
    if( index < 0 )
        m_parent->appendChild( this );
    else
        m_parent->insertChild( index, this );
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

PlaylistItem::PlaylistItem( SourcesModel* mdl, SourceTreeItem* parent, const playlist_ptr& pl, int index )
    : SourceTreeItem( mdl, parent, SourcesModel::StaticPlaylist, index )
    , m_loaded( false )
    , m_playlist( pl )
{
    connect( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
              SLOT( onPlaylistLoaded( Tomahawk::PlaylistRevision ) ), Qt::QueuedConnection );
    connect( pl.data(), SIGNAL( changed() ),
              SIGNAL( updated() ), Qt::QueuedConnection );

    if( ViewManager::instance()->pageForPlaylist( pl ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForPlaylist( pl ) );
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
PlaylistItem::flags() const
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
    ViewPage* p = ViewManager::instance()->show( m_playlist );
    model()->linkSourceItemToPage( this, p );
}

void
PlaylistItem::setLoaded( bool loaded )
{
    m_loaded = loaded;
}

bool
PlaylistItem::willAcceptDrag( const QMimeData* data ) const
{
    return !m_playlist.isNull() && m_playlist->author()->isLocal();
}


bool
PlaylistItem::dropMimeData( const QMimeData* data, Qt::DropAction action )
{
    if( data->hasFormat( "application/tomahawk.query.list" ) ) {
        if ( !m_playlist.isNull() && m_playlist->author()->isLocal() ) {

            QByteArray itemData = data->data( "application/tomahawk.query.list" );
            QDataStream stream( &itemData, QIODevice::ReadOnly );
            QList< Tomahawk::query_ptr > queries;

            while ( !stream.atEnd() )
            {
                qlonglong qptr;
                stream >> qptr;

                Tomahawk::query_ptr* query = reinterpret_cast<Tomahawk::query_ptr*>(qptr);
                if ( query && !query->isNull() )
                {
                    qDebug() << "Dropped query item:" << query->data()->artist() << "-" << query->data()->track();
                    queries << *query;
                }
            }

            qDebug() << "on playlist:" << m_playlist->title() << m_playlist->guid();

            // TODO do we need to use this in the refactor?
            //                     QString rev = item->currentlyLoadedPlaylistRevision( playlist->guid() );
            m_playlist->addEntries( queries, m_playlist->currentrevision() );

            return true;
        }
    }

    return false;
}

QIcon
PlaylistItem::icon() const
{
    return QIcon( RESPATH "images/playlist-icon.png" );
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
    ViewManager::instance()->show( m_playlist );
}


void
DynPlaylistItem::onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision revision )
{
    setLoaded( true );
}*/

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
    switch( m_categoryType ) {
    case SourcesModel::PlaylistsCategory:
        return tr( "New Playlist" );
    case SourcesModel::StationsCategory:
        return tr( "New Station" );
    }

    return QString();
}

void
CategoryAddItem::activate()
{
    switch( m_categoryType ) {
        case SourcesModel::PlaylistsCategory:
            // only show if none is shown yet
            if( !ViewManager::instance()->isNewPlaylistPageVisible() ) {
                ViewPage* p = ViewManager::instance()->show( new NewPlaylistWidget() );
                model()->linkSourceItemToPage( this, p );
            }
            break;
        case SourcesModel::StationsCategory:
            // TODO
            break;
    }
}

Qt::ItemFlags
CategoryAddItem::flags() const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QIcon
CategoryAddItem::icon() const
{
    return QIcon( RESPATH "images/add.png" );
}

// CategoryItem

CategoryItem::CategoryItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType category, bool showAddItem )
    : SourceTreeItem( model, parent, SourcesModel::Category )
    , m_category( category )
    , m_addItem( 0 )
    , m_showAdd( showAddItem )
{
    // in the constructor we're still being added to the parent, so we don't exist to have rows addded yet. so this is safe.
//     beginRowsAdded( 0, 0 );
    if( m_showAdd ) {
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
    if( m_showAdd ) // if there's an add item, add it before that
        curCount--;
    beginRowsAdded( curCount, curCount + items.size() - 1 );
    foreach( SourceTreeItem* item, items ) {
        int index = m_showAdd ? children().count() - 1 : children().count();
        insertChild( children().count() - 1, item );
    }
    endRowsAdded();
}


/// CollectionItem

CollectionItem::CollectionItem(  SourcesModel* mdl, SourceTreeItem* parent, const Tomahawk::source_ptr& source )
    : SourceTreeItem( mdl, parent, SourcesModel::Collection )
    , m_source( source )
    , m_playlists( 0 )
    , m_stations( 0 )
{
    if( m_source.isNull() ) { // super collection
        return;
    }
    // create category items if there are playlists to show, or stations to show
    QList< playlist_ptr > playlists = source->collection()->playlists();

    if( !playlists.isEmpty() || source->isLocal() ) {
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source->isLocal() );
        // ugh :( we're being added by the model, no need to notify for added rows now
//         m_playlists->blockSignals( true );
        onPlaylistsAdded( playlists );
//         m_playlists->blockSignals( false );
    }

    // TODO always show for now, till we actually support stations
//     m_stations = new CategoryItem( model(), this, SourcesModel::StationsCategory, source->isLocal() );

    if( ViewManager::instance()->pageForCollection( source->collection() ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForCollection( source->collection() ) );

    // HACK to load only for now
    source->collection()->dynamicPlaylists();

    connect( source.data(), SIGNAL( stats( QVariantMap ) ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( stateChanged() ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( offline() ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( online() ), this, SIGNAL( updated() ) );

    connect( source->collection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsAdded( QList<Tomahawk::playlist_ptr> ) ), Qt::QueuedConnection );
    connect( source->collection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsDeleted( QList<Tomahawk::playlist_ptr> ) ), Qt::QueuedConnection );
}

Tomahawk::source_ptr
CollectionItem::source() const
{
    return m_source;
}

QString
CollectionItem::text() const
{
    return m_source.isNull() ? tr( "Super Collection" ) : m_source->friendlyName();
}

void
CollectionItem::activate()
{
    if( source().isNull() ) {
        ViewPage* p = ViewManager::instance()->showSuperCollection();
        model()->linkSourceItemToPage( this, p );
    } else {
        ViewPage* p = ViewManager::instance()->show( source()->collection() );
        model()->linkSourceItemToPage( this, p );
    }
}

QIcon
CollectionItem::icon() const
{
    if( m_source.isNull() )
        return QIcon( RESPATH "images/supercollection.png" );
    else
        return QIcon( RESPATH "images/user-avatar.png" );
}


void
CollectionItem::onPlaylistsAdded( const QList< playlist_ptr >& playlists )
{
    if( playlists.isEmpty() )
        return;

    if( !m_playlists ) { // add the category too
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source()->isLocal() );
    }

    QList< SourceTreeItem* > items;
    int addOffset = playlists.first()->author()->isLocal() ? 1 : 0;

    int from = m_playlists->children().count() - addOffset;
    m_playlists->beginRowsAdded( from, from + playlists.count() - 1 );
    foreach( const playlist_ptr& p, playlists )
    {
        PlaylistItem* plItem = new PlaylistItem( model(), m_playlists, p, m_playlists->children().count() - addOffset );
        qDebug() << "Playlist added:" << p->title() << p->creator() << p->info();
        p->loadRevision();
        items << plItem;
    }
    m_playlists->endRowsAdded();
}

void
CollectionItem::onPlaylistsDeleted( const QList< playlist_ptr >& playlists )
{
    Q_ASSERT( m_playlists ); // How can we delete playlists if we have none?
    QList< SourceTreeItem* > items;
    foreach( const playlist_ptr& playlist, playlists ) {
        int curCount = m_playlists->children().count();
        for( int i = 0; i < curCount; i++ ) {
            PlaylistItem* pl = qobject_cast< PlaylistItem* >( m_playlists->children().at( i ) );
            if( pl && pl->playlist() == playlist ) {
                m_playlists->beginRowsRemoved( i, i );
                m_playlists->removeChild( pl );
                m_playlists->endRowsRemoved();
                break;
            }
        }
    }
}

/// Generic page item
GenericPageItem::GenericPageItem( SourcesModel* model, SourceTreeItem* parent, const QString& text, const QIcon& icon )
    : SourceTreeItem( model, parent, SourcesModel::GenericPage )
    , m_icon( icon )
    , m_text( text )
{

}

GenericPageItem::~GenericPageItem()
{

}

void
GenericPageItem::activate()
{
    emit activated();
}

QString
GenericPageItem::text() const
{
    return m_text;
}

QIcon
GenericPageItem::icon() const
{
    return m_icon;
}


bool
GenericPageItem::willAcceptDrag(const QMimeData* data) const
{
    return false;
}

