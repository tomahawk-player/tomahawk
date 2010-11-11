#include "sourcetreeitem.h"

#include <QDebug>
#include <QTreeView>

#include "tomahawk/collection.h"
#include "tomahawk/playlist.h"
#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;


SourceTreeItem::SourceTreeItem( const source_ptr& source, QObject* parent )
    : QObject( parent )
    , m_source( source )
{
    QStandardItem* item = new QStandardItem( "" );
    item->setEditable( false );
    item->setData( 0, Qt::UserRole + 1 );
    item->setData( (qlonglong)this, Qt::UserRole + 2 );
    m_columns << item;

    if ( !source.isNull() )
    {
        onPlaylistsAdded( source->collection()->playlists() );

        connect( source->collection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ),
                                                SLOT( onPlaylistsAdded( QList<Tomahawk::playlist_ptr> ) ) );

        connect( source->collection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ),
                                                SLOT( onPlaylistsDeleted( QList<Tomahawk::playlist_ptr> ) ) );
    }

    m_widget = new SourceTreeItemWidget( source, (QWidget*)parent->parent() );
    connect( m_widget, SIGNAL( clicked() ), SLOT( onClicked() ) );
}


void
SourceTreeItem::onClicked()
{
    emit clicked( m_columns.at( 0 )->index() );
}


void
SourceTreeItem::onOnline()
{
    m_widget->onOnline();
}


void
SourceTreeItem::onOffline()
{
    m_widget->onOffline();
}


void
SourceTreeItem::onPlaylistsAdded( const QList<playlist_ptr>& playlists )
{
    // const-ness is important for getting the right pointer!
    foreach( const playlist_ptr& p, playlists )
    {
        m_playlists.append( p );
        qlonglong ptr = qlonglong( &m_playlists.last() );
        qDebug() << "Playlist added:" << p->title() << p->creator() << p->info() << ptr;

        connect( p.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
                             SLOT( onPlaylistLoaded( Tomahawk::PlaylistRevision ) ),
                 Qt::QueuedConnection);

        QStandardItem* subitem = new QStandardItem( p->title() );
        subitem->setIcon( QIcon( RESPATH "images/playlist-icon.png" ) );
        subitem->setEditable( false );
        subitem->setEnabled( false );
        subitem->setData( ptr, Qt::UserRole + 3 );
        subitem->setData( 1, Qt::UserRole + 1 );
        subitem->setData( (qlonglong)this, Qt::UserRole + 2 );

        m_columns.at( 0 )->appendRow( subitem );
        ((QTreeView*)parent()->parent())->expandAll();

        p->loadRevision();
    }
}


void
SourceTreeItem::onPlaylistsDeleted( const QList<playlist_ptr>& playlists )
{
    // const-ness is important for getting the right pointer!
    foreach( const playlist_ptr& p, playlists )
    {
        qlonglong ptr = qlonglong( p.data() );
        qDebug() << "Playlist removed:" << p->title() << p->creator() << p->info() << ptr;

        QStandardItem* item = m_columns.at( 0 );
        int rows = item->rowCount();
        for ( int i = rows - 1; i >= 0; i-- )
        {
            QStandardItem* pi = item->child( i );
            qlonglong piptr = pi->data( Qt::UserRole + 3 ).toLongLong();
            playlist_ptr* pl = reinterpret_cast<playlist_ptr*>(piptr);
            int type = pi->data( Qt::UserRole + 1 ).toInt();

            if ( type == 1 && ptr == qlonglong( pl->data() ) )
            {
                m_playlists.removeAll( p );
                item->removeRow( i );
            }
        }
    }
}


void
SourceTreeItem::onPlaylistLoaded( Tomahawk::PlaylistRevision revision )
{
    qlonglong ptr = qlonglong( sender() );
    //qDebug() << "sender ptr:" << ptr;

    QStandardItem* item = m_columns.at( 0 );
    int rows = item->rowCount();
    for ( int i = 0; i < rows; i++ )
    {
        QStandardItem* pi = item->child( i );
        qlonglong piptr = pi->data( Qt::UserRole + 3 ).toLongLong();
        playlist_ptr* pl = reinterpret_cast<playlist_ptr*>(piptr);
        int type = pi->data( Qt::UserRole + 1 ).toInt();

        if ( type == 1 && ptr == qlonglong( pl->data() ) )
        {
            //qDebug() << "Found playlist!";
            pi->setEnabled( true );
            m_current_revisions.insert( pl->data()->guid(), revision.revisionguid );
        }
    }
}
