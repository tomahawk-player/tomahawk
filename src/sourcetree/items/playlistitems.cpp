/*
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "playlistitems.h"

#include "utils/tomahawkutils.h"
#include "viewmanager.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "categoryitems.h"
#include "collectionitem.h"

#include <QMimeData>

using namespace Tomahawk;


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


int
PlaylistItem::peerSortValue() const
{
    return m_playlist->createdOn();
}


Qt::ItemFlags
PlaylistItem::flags() const
{
    Qt::ItemFlags flags = SourceTreeItem::flags();
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if ( !m_loaded )
        flags &= !Qt::ItemIsEnabled;
    if ( playlist()->author()->isLocal() )
        flags |= Qt::ItemIsEditable;

    if ( playlist()->busy() )
    {
        flags &= !Qt::ItemIsEnabled;
        flags &= !Qt::ItemIsEditable;
        flags &= !Qt::ItemIsDropEnabled;
    }

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
    QList< Tomahawk::query_ptr > queries;

    if ( data->hasFormat( "application/tomahawk.playlist.id" ) &&
        data->data( "application/tomahawk.playlist.id" ) == m_playlist->guid() )
        return false; // don't allow dropping on ourselves

    if ( data->hasFormat( "application/tomahawk.result.list" ) )
    {
        QByteArray itemData = data->data( "application/tomahawk.result.list" );
        QDataStream stream( &itemData, QIODevice::ReadOnly );

        while ( !stream.atEnd() )
        {
            qlonglong qptr;
            stream >> qptr;

            Tomahawk::result_ptr* result = reinterpret_cast<Tomahawk::result_ptr*>(qptr);
            if ( result && !result->isNull() )
            {
                qDebug() << "Dropped result item:" << result->data()->artist() << "-" << result->data()->track();
                queries << result->data()->toQuery();
            }
        }
    }

    if ( data->hasFormat( "application/tomahawk.query.list" ) )
    {
        QByteArray itemData = data->data( "application/tomahawk.query.list" );
        QDataStream stream( &itemData, QIODevice::ReadOnly );

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
    }

    if ( queries.count() && !m_playlist.isNull() && m_playlist->author()->isLocal() )
    {
        qDebug() << "on playlist:" << m_playlist->title() << m_playlist->guid() << m_playlist->currentrevision();

        m_playlist->addEntries( queries, m_playlist->currentrevision() );

        return true;
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


int
DynamicPlaylistItem::peerSortValue() const
{
    return m_dynplaylist->createdOn();
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
