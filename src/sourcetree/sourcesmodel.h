
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


#ifndef SOURCESMODEL_H
#define SOURCESMODEL_H

#include <QModelIndex>
#include <QStringList>

#include "typedefs.h"

class QMimeData;

class SourceTreeItem;

namespace Tomahawk {
    class Source;
    class Playlist;
    class ViewPage;
}

class SourcesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum RowType {
        Invalid = -1,

        Collection = 0,

        Category = 1,
        CategoryAdd = 2,

        StaticPlaylist = 3,
        AutomaticPlaylist = 4,
        Station = 5,

        GenericPage = 6
    };

    enum CategoryType {
        PlaylistsCategory = 0,
        StationsCategory = 1
    };

    enum Roles {
        SourceTreeItemRole      = Qt::UserRole + 10,
        SourceTreeItemTypeRole  = Qt::UserRole + 11,
        SortRole                = Qt::UserRole + 12
    };

    SourcesModel( QObject* parent = 0 );
    virtual ~SourcesModel();

    static QString rowTypeToString( RowType type );

    // reimplemented from QAIM
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    virtual QStringList mimeTypes() const;
    virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

    void appendItem( const Tomahawk::source_ptr& source );
    bool removeItem( const Tomahawk::source_ptr& source );

    void linkSourceItemToPage( SourceTreeItem* item, Tomahawk::ViewPage* p );

    QModelIndex indexFromItem( SourceTreeItem* item ) const;

public slots:
    void loadSources();

    void itemUpdated();
    void onItemRowsAddedBegin( int first, int last );
    void onItemRowsAddedDone();
    void onItemRowsRemovedBegin( int first, int last );
    void onItemRowsRemovedDone();

    void viewPageActivated( Tomahawk::ViewPage* );

    void itemSelectRequest( SourceTreeItem* item );
    void itemExpandRequest( SourceTreeItem* item );

signals:
    void selectRequest( const QModelIndex& idx );
    void expandRequest( const QModelIndex& idx );

private slots:
    void onSourcesAdded( const QList<Tomahawk::source_ptr>& sources );
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onSourceRemoved( const Tomahawk::source_ptr& source );

private:
    SourceTreeItem* itemFromIndex( const QModelIndex& idx ) const;
    int rowForItem( SourceTreeItem* item ) const;
    bool activatePlaylistPage( Tomahawk::ViewPage* p, SourceTreeItem* i );

    SourceTreeItem* m_rootItem;

    QHash< Tomahawk::ViewPage*, SourceTreeItem* > m_sourceTreeLinks;
    Tomahawk::ViewPage* m_viewPageDelayedCacheItem;
};

#endif // SOURCESMODEL_H
