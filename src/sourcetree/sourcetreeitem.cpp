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

bool
PlaylistItem::setData(const QVariant& v, bool role)
{
    if( m_playlist->author()->isLocal() ) {
        m_playlist->rename( v.toString() );

        return true;
    }
    return false;
}

DynamicPlaylistItem::DynamicPlaylistItem( SourcesModel* mdl, SourceTreeItem* parent, const dynplaylist_ptr& pl, int index )
    : PlaylistItem( mdl, parent, pl.staticCast< Playlist >(), index )
    , m_dynplaylist( pl )
{
    setRowType( m_dynplaylist->mode() == Static ? SourcesModel::AutomaticPlaylist : SourcesModel::Station );

    connect( pl.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ),
             SLOT( onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision ) ), Qt::QueuedConnection );

    if( ViewManager::instance()->pageForDynPlaylist( pl ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForDynPlaylist( pl ) );
}

DynamicPlaylistItem::~DynamicPlaylistItem()
{
}

void
DynamicPlaylistItem::activate()
{
    ViewPage* p = ViewManager::instance()->show( m_dynplaylist );
    model()->linkSourceItemToPage( this, p );
}

void
DynamicPlaylistItem::onDynamicPlaylistLoaded( DynamicPlaylistRevision revision )
{
    setLoaded( true );
    checkReparentHackNeeded( revision );
    // END HACK
    emit updated();
}

void
DynamicPlaylistItem::checkReparentHackNeeded( const DynamicPlaylistRevision& revision )
{
    // HACK HACK HACK  workaround for an ugly hack where we have to be compatible with older tomahawks (pre-0.1.0) that created dynplaylists as OnDemand then changed them to Static if they were static.
    //  we need to reparent this playlist to the correct category :-/.
    CategoryItem* cat = qobject_cast< CategoryItem* >( parent() );
    qDebug() << "with category" << cat;
    if( cat ) qDebug() << "and cat type:" << cat->categoryType();
    if( cat ) {
        CategoryItem* from = cat;
        CategoryItem* to = 0;
        if( cat->categoryType() == SourcesModel::PlaylistsCategory && revision.mode == OnDemand ) { // WRONG
            CollectionItem* col = qobject_cast< CollectionItem* >( cat->parent() );
            to = col->stationsCategory();
            if( !to ) { // you have got to be fucking kidding me
                int fme = col->children().count();
                col->beginRowsAdded( fme, fme );
                to = new CategoryItem( model(), col, SourcesModel::StationsCategory, false );
                col->appendChild( to ); // we f'ing know it's not local b/c we're not getting into this mess ourselves
                col->endRowsAdded();
                col->setStationsCategory( to );
            }
        } else if( cat->categoryType() == SourcesModel::StationsCategory && revision.mode == Static ) { // WRONG
            CollectionItem* col = qobject_cast< CollectionItem* >( cat->parent() );
            to = col->playlistsCategory();
            qDebug() << "TRYING TO HACK TO:" << to;
            if( !to ) { // you have got to be fucking kidding me
                int fme = col->children().count();
                col->beginRowsAdded( fme, fme );
                to = new CategoryItem( model(), col, SourcesModel::PlaylistsCategory, false );
                col->appendChild( to ); // we f'ing know it's not local b/c we're not getting into this mess ourselves
                col->endRowsAdded();
                col->setPlaylistsCategory( to );
            }
        }
        if( to ) {
            qDebug() << "HACKING! moving dynamic playlist from" << from->text() << "to:" << to->text();
            // remove and add
            int idx = from->children().indexOf( this );
            from->beginRowsRemoved( idx, idx );
            from->removeChild( this );
            from->endRowsRemoved();

            idx = to->children().count();
            to->beginRowsAdded( idx, idx );
            to->appendChild( this );
            to->endRowsAdded();

            setParentItem( to );
        }
    }
}


dynplaylist_ptr
DynamicPlaylistItem::dynPlaylist() const
{
    return m_dynplaylist;
}

QString
DynamicPlaylistItem::text() const
{
    return m_dynplaylist->title();
}

bool
DynamicPlaylistItem::willAcceptDrag( const QMimeData* data ) const
{
    return false;
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
    switch( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            // only show if none is shown yet
            if( !ViewManager::instance()->isNewPlaylistPageVisible() ) {
                ViewPage* p = ViewManager::instance()->show( new NewPlaylistWidget() );
                model()->linkSourceItemToPage( this, p );
            }
            break;
        case SourcesModel::StationsCategory:
            APP->mainWindow()->createStation();
            break;
    }
}

Qt::ItemFlags
CategoryAddItem::flags() const
{
    switch( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        case SourcesModel::StationsCategory:
        default:
            return Qt::ItemIsEnabled;
            break;
    }
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
    QList< dynplaylist_ptr > autoplaylists = source->collection()->autoPlaylists();
    QList< dynplaylist_ptr > stations = source->collection()->stations();

    if( !playlists.isEmpty() || !autoplaylists.isEmpty() || source->isLocal() ) {
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source->isLocal() );
        onPlaylistsAdded( playlists );
        onAutoPlaylistsAdded( autoplaylists );
    }
    if( !stations.isEmpty() || source->isLocal() ) {
        m_stations = new CategoryItem( model(), this, SourcesModel::StationsCategory, source->isLocal() );
        onStationsAdded( stations );
    }

    if( ViewManager::instance()->pageForCollection( source->collection() ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForCollection( source->collection() ) );

    // load auto playlists and stations!

    connect( source.data(), SIGNAL( stats( QVariantMap ) ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( stateChanged() ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( offline() ), this, SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( online() ), this, SIGNAL( updated() ) );

    connect( source->collection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsAdded( QList<Tomahawk::playlist_ptr> ) ), Qt::QueuedConnection );
    connect( source->collection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsDeleted( QList<Tomahawk::playlist_ptr> ) ), Qt::QueuedConnection );

    connect( source->collection().data(), SIGNAL( autoPlaylistsAdded( QList< Tomahawk::dynplaylist_ptr > ) ),
             SLOT( onAutoPlaylistsAdded( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );
    connect( source->collection().data(), SIGNAL( autoPlaylistsDeleted( QList<Tomahawk::dynplaylist_ptr> ) ),
             SLOT( onAutoPlaylistsDeleted( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );

    connect( source->collection().data(), SIGNAL( stationsAdded( QList<Tomahawk::dynplaylist_ptr> ) ),
             SLOT( onStationsAdded( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );
    connect( source->collection().data(), SIGNAL( stationsDeleted( QList<Tomahawk::dynplaylist_ptr> ) ),
             SLOT( onStationsDeleted( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );
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
CollectionItem::playlistsAddedInternal( SourceTreeItem* parent, const QList< dynplaylist_ptr >& playlists )
{
    QList< SourceTreeItem* > items;
    int addOffset = playlists.first()->author()->isLocal() ? 1 : 0;

    int from = parent->children().count() - addOffset;
    parent->beginRowsAdded( from, from + playlists.count() - 1 );
    foreach( const dynplaylist_ptr& p, playlists )
    {
        DynamicPlaylistItem* plItem = new DynamicPlaylistItem( model(), parent, p, parent->children().count() - addOffset );
        qDebug() << "Dynamic Playlist added:" << p->title() << p->creator() << p->info();
        p->loadRevision();
        items << plItem;
    }
    parent->endRowsAdded();
}

template< typename T >
void
CollectionItem::playlistsDeletedInternal( SourceTreeItem* parent, const QList< T >& playlists )
{
    Q_ASSERT( parent ); // How can we delete playlists if we have none?
    QList< SourceTreeItem* > items;
    foreach( const T& playlist, playlists ) {
        int curCount = parent->children().count();
        for( int i = 0; i < curCount; i++ ) {
            PlaylistItem* pl = qobject_cast< PlaylistItem* >( parent->children().at( i ) );
            if( pl && pl->playlist() == playlist ) {
                parent->beginRowsRemoved( i, i );
                parent->removeChild( pl );
                parent->endRowsRemoved();
                break;
            }
        }
    }
}


void
CollectionItem::onPlaylistsAdded( const QList< playlist_ptr >& playlists )
{
    if( playlists.isEmpty() )
        return;

    if( !m_playlists ) { // add the category too
        int cur = children().count();
        beginRowsAdded( cur, cur );
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source()->isLocal() );
        endRowsAdded();
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
    playlistsDeletedInternal( m_playlists, playlists );
}

void
CollectionItem::onAutoPlaylistsAdded( const QList< dynplaylist_ptr >& playlists )
{
    if( playlists.isEmpty() )
        return;

    if( !m_playlists ) { // add the category too
        int cur = children().count();
        beginRowsAdded( cur, cur );
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source()->isLocal() );
        endRowsAdded();
    }

    playlistsAddedInternal( m_playlists, playlists );
}

void
CollectionItem::onAutoPlaylistsDeleted( const QList< dynplaylist_ptr >& playlists )
{
    if( !m_playlists )
        qDebug() << "NO playlist category item for a deleting playlist..";

    playlistsDeletedInternal( m_playlists, playlists );
}

void
CollectionItem::onStationsAdded( const QList< dynplaylist_ptr >& stations )
{
    if( stations.isEmpty() )
        return;

    if( !m_stations ) { // add the category too
        int cur = children().count();
        beginRowsAdded( cur, cur );
        m_stations = new CategoryItem( model(), this, SourcesModel::StationsCategory, source()->isLocal() );
        endRowsAdded();
    }

    playlistsAddedInternal( m_stations, stations );
}

void
CollectionItem::onStationsDeleted( const QList< dynplaylist_ptr >& stations )
{
    playlistsDeletedInternal( m_stations, stations );
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

