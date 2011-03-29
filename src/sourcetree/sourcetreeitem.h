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


#ifndef SOURCETREEITEM_H
#define SOURCETREEITEM_H

#include "sourcesmodel.h"
#include "typedefs.h"
#include "playlist.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "source.h"

class SourceTreeItem : public QObject 
{
    Q_OBJECT
public:
    SourceTreeItem() : m_type( SourcesModel::Invalid ), m_parent( 0 ), m_model( 0 ) {}
    SourceTreeItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::RowType thisType );
    virtual ~SourceTreeItem();
    
    // generic info used by the tree model 
    SourcesModel::RowType type() const { return m_type; }
    SourceTreeItem* parent() const { return m_parent; }
    SourcesModel* model() const { return m_model; }

    QList< SourceTreeItem* > children() const { return m_children; }
    void appendChild( SourceTreeItem* item ) { m_children.append( item ); }
    void removeChild( SourceTreeItem* item ) { m_children.removeAll( item ); }
    
    // varies depending on the type of the item
    virtual QString text() const { return QString(); }
    virtual Qt::ItemFlags flags() { return Qt::ItemIsSelectable | Qt::ItemIsEnabled; }
    virtual void activate() {}
    
    /// don't call me unless you are a sourcetreeitem. i prefer this to making everyone a friend
    void beginRowsAdded( int from, int num ) { emit beginChildRowsAdded( from, num ); }
    void endRowsAdded() { emit childRowsAdded(); }
    void beginRowsRemoved( int from, int num ) { emit beginChildRowsRemoved( from, num ); }
    void endRowsRemoved() { emit childRowsRemoved(); }
signals:
    void updated();
    
    void beginChildRowsAdded( int fromRow, int num );
    void childRowsAdded();
    
    void beginChildRowsRemoved( int fromRow, int num );
    void childRowsRemoved();
private:
    SourcesModel::RowType m_type;
    
    SourceTreeItem* m_parent;
    QList< SourceTreeItem* > m_children;
    SourcesModel* m_model;
};


class CategoryItem : public SourceTreeItem
{
    Q_OBJECT
public:
    CategoryItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType category ) : SourceTreeItem( model, parent, SourcesModel::Category ), m_category( category ) {}
    
    virtual QString text() const { 
        switch( m_category )
        {
            case SourcesModel::PlaylistsCategory:
                return tr( "Playlists" );
            case SourcesModel::StationsCategory:
                return tr( "Stations" );
        }
        return QString();
    }
    virtual void activate();
    virtual Qt::ItemFlags flags() { return Qt::ItemIsEnabled; }
    
    SourcesModel::CategoryType categoryType() { return m_category; }
    
private:
    SourcesModel::CategoryType m_category;
};


class CollectionItem : public SourceTreeItem
{
    Q_OBJECT
public:
    CollectionItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::source_ptr& source );
    
    virtual QString text() const;
    Tomahawk::source_ptr source() const;
    virtual void activate();
    
private slots:
    void onPlaylistsAdded( const QList<Tomahawk::playlist_ptr>& playlists );
    void onPlaylistsDeleted( const QList<Tomahawk::playlist_ptr>& playlists );
    
private:
    Tomahawk::source_ptr m_source;
    
    CategoryItem* m_playlists;
    CategoryItem* m_stations;
};
class PlaylistItem : public SourceTreeItem
{
    Q_OBJECT
public:
    PlaylistItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::playlist_ptr& pl );
    
    virtual QString text() const;
    virtual Tomahawk::playlist_ptr playlist() const;
    virtual Qt::ItemFlags flags();
    virtual void activate();
    
protected:
    void setLoaded( bool loaded );
    
private slots:
    void onPlaylistLoaded( Tomahawk::PlaylistRevision revision );
    void onPlaylistChanged();
    
private:
    bool m_loaded;
    Tomahawk::playlist_ptr m_playlist;
};

/*
class DynPlaylistItem : public PlaylistItem
{
    Q_OBJECT
public:
    DynPlaylistItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::dynplaylist_ptr& pl );
    
    virtual QString text() const;
    virtual Tomahawk::playlist_ptr playlist() const;
//     Tomahawk::dynplaylist_ptr playlist() const;
    virtual void activate();
    
private slots:
    void onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision revision );
    
private:
    Tomahawk::dynplaylist_ptr m_dynplaylist;
};*/

Q_DECLARE_METATYPE( SourceTreeItem* );

#endif // SOURCETREEITEM_H
