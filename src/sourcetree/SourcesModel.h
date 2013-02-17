/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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


#ifndef SOURCESMODEL_H
#define SOURCESMODEL_H

#include "Typedefs.h"
#include "Source.h"

#include <QModelIndex>
#include <QStringList>
#include <QList>
#include <QAction>

class QMimeData;

class SourceTreeItem;
class GroupItem;

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
        Divider = 9,

        Collection = 0,
        Group = 8,

        Category = 1,
        CategoryAdd = 2,

        StaticPlaylist = 3,
        AutomaticPlaylist = 4,
        Station = 5,

        GenericPage = 6,
        TemporaryPage = 7,
        LovedTracksPage = 10,

        ScriptCollection = 11
    };

    enum CategoryType {
        PlaylistsCategory = 0,
        StationsCategory = 1
    };

    enum Roles {
        SourceTreeItemRole      = Qt::UserRole + 10,
        SourceTreeItemTypeRole  = Qt::UserRole + 11,
        SortRole                = Qt::UserRole + 12,
        IDRole                  = Qt::UserRole + 13,
        LatchedOnRole           = Qt::UserRole + 14,
        LatchedRealtimeRole     = Qt::UserRole + 15,
        CustomActionRole        = Qt::UserRole + 16 // QList< QAction* >
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

    void appendGroups();

    void appendItem( const Tomahawk::source_ptr& source );
    bool removeItem( const Tomahawk::source_ptr& source );

    void linkSourceItemToPage( SourceTreeItem* item, Tomahawk::ViewPage* p );
    void removeSourceItemLink( SourceTreeItem* item );

    QModelIndex indexFromItem( SourceTreeItem* item ) const;

    QList< Tomahawk::source_ptr > sourcesWithViewPage() const;

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
    void itemToggleExpandRequest( SourceTreeItem* item );

signals:
    void selectRequest( const QPersistentModelIndex& idx );
    void expandRequest( const QPersistentModelIndex& idx );
    void toggleExpandRequest( const QPersistentModelIndex& idx );

private slots:
    void onSourcesAdded( const QList<Tomahawk::source_ptr>& sources );
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onSourceRemoved( const Tomahawk::source_ptr& source );

    void onScriptCollectionAdded( const Tomahawk::collection_ptr& collection );
    void onScriptCollectionRemoved( const Tomahawk::collection_ptr& collection );

    Tomahawk::ViewPage* scriptCollectionClicked( const Tomahawk::collection_ptr& collection );
    Tomahawk::ViewPage* getScriptCollectionPage( const Tomahawk::collection_ptr& collection ) const;

    void onWidgetDestroyed( QWidget* w );

private:
    SourceTreeItem* itemFromIndex( const QModelIndex& idx ) const;
    int rowForItem( SourceTreeItem* item ) const;
    SourceTreeItem* activatePlaylistPage( Tomahawk::ViewPage* p, SourceTreeItem* i );

    SourceTreeItem* m_rootItem;
    GroupItem* m_collectionsGroup;
    GroupItem* m_myMusicGroup;
    GroupItem* m_cloudGroup;

    QList< Tomahawk::source_ptr > m_sourcesWithViewPage;
    QHash< Tomahawk::source_ptr, SourceTreeItem* > m_sourcesWithViewPageItems;
    QHash< Tomahawk::collection_ptr, SourceTreeItem* > m_scriptCollections;
    QHash< Tomahawk::collection_ptr, Tomahawk::ViewPage* > m_scriptCollectionPages;

    QHash< Tomahawk::ViewPage*, SourceTreeItem* > m_sourceTreeLinks;
    Tomahawk::ViewPage* m_viewPageDelayedCacheItem;
};

Q_DECLARE_METATYPE( QList< QAction* > )

#endif // SOURCESMODEL_H
