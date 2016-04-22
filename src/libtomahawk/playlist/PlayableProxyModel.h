/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TRACKPROXYMODEL_H
#define TRACKPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "PlaylistInterface.h"
#include "playlist/PlayableModel.h"

#include "DllMacro.h"

class PlayableProxyModelFilterMemo
{
public:
    PlayableProxyModelFilterMemo()
    {
        // First element always has no predecessors.
        // TODO C++11: Make this a constexpr using initializer lists.
        visibilty.push_back( 0 );
    }

    virtual ~PlayableProxyModelFilterMemo() {}
    std::vector<int> visibilty;
};

class DLLEXPORT PlayableProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    enum PlayableItemStyle
    { Detailed = 0, SingleColumn = 1, Collection = 2, Locker = 3 };

    enum PlayableProxyModelRole
    { StyleRole = Qt::UserRole + 1, TypeRole };

    explicit PlayableProxyModel ( QObject* parent = 0 );
    virtual ~PlayableProxyModel() {}

    virtual QString guid() const;

    PlayableModel* sourceModel() const { return m_model; }
    virtual void setSourcePlayableModel( PlayableModel* sourceModel );
    virtual void setSourceModel( QAbstractItemModel* model ) override;

    virtual bool isLoading() const;

    PlayableProxyModel::PlayableItemStyle style() const { return m_style; }
    void setStyle( PlayableProxyModel::PlayableItemStyle style ) { m_style = style; }

    virtual QPersistentModelIndex currentIndex() const;
    virtual void setCurrentIndex( const QModelIndex& index );

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QModelIndexList& indexes );
    virtual void removeIndexes( const QList< QPersistentModelIndex >& indexes );

    bool showOfflineResults() const { return m_showOfflineResults; }
    void setShowOfflineResults( bool b );

    bool hideDupeItems() const { return m_hideDupeItems; }
    void setHideDupeItems( bool b );

    int maxVisibleItems() const { return m_maxVisibleItems; }
    void setMaxVisibleItems( int items );

    PlayableItem* itemFromIndex( const QModelIndex& index ) const { return sourceModel()->itemFromIndex( index ); }
    PlayableItem* itemFromQuery( const Tomahawk::query_ptr& query ) const { return sourceModel()->itemFromQuery( query ); }
    PlayableItem* itemFromResult( const Tomahawk::result_ptr& result ) const { return sourceModel()->itemFromResult( result ); }

    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;
    void setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface );

    QList< double > columnWeights() const;

    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

    virtual void setFilter( const QString& pattern );
    virtual void updateDetailedInfo( const QModelIndex& index );

    int mapSourceColumnToColumn( PlayableModel::Columns column );

signals:
    void filterChanged( const QString& filter );

    void filteringStarted();
    void filteringFinished();

    void loadingStarted();
    void loadingFinished();

    void indexPlayable( const QModelIndex& index );
    void indexResolved( const QModelIndex& index );
    void currentIndexChanged( const QModelIndex& newIndex, const QModelIndex& oldIndex );

    void itemCountChanged( unsigned int items );

    void expandRequest( const QPersistentModelIndex& index );
    void selectRequest( const QPersistentModelIndex& index );

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const Q_DECL_OVERRIDE;
    virtual bool lessThan( const QModelIndex& left, const QModelIndex& right ) const override;

    Tomahawk::playlistinterface_ptr m_playlistInterface;

private slots:
    void onIndexPlayable( const QModelIndex& index );
    void onIndexResolved( const QModelIndex& index );

    void expandRequested( const QPersistentModelIndex& index );
    void selectRequested( const QPersistentModelIndex& index );
    void onCurrentIndexChanged( const QModelIndex& newIndex, const QModelIndex& oldIndex );

private:
    bool filterAcceptsRowInternal( int sourceRow, PlayableItem* pi, const QModelIndex& sourceParent, PlayableProxyModelFilterMemo& memo ) const;
    bool nameFilterAcceptsRow( int sourceRow, PlayableItem* pi, const QModelIndex& sourceParent ) const;
    bool dupeFilterAcceptsRow( int sourceRow, PlayableItem* pi, const QModelIndex& sourceParent, PlayableProxyModelFilterMemo& memo ) const;
    bool visibilityFilterAcceptsRow( int sourceRow, const QModelIndex& sourceParent, PlayableProxyModelFilterMemo& memo ) const;

    bool lessThan( int column, const Tomahawk::query_ptr& left, const Tomahawk::query_ptr& right ) const;
    bool lessThan( const Tomahawk::album_ptr& album1, const Tomahawk::album_ptr& album2 ) const;

    QPointer<PlayableModel> m_model;

    bool m_showOfflineResults;
    bool m_hideEmptyParents;
    bool m_hideDupeItems;
    int m_maxVisibleItems;

    QHash< PlayableItemStyle, QList<PlayableModel::Columns> > m_headerStyle;
    PlayableItemStyle m_style;
};

#endif // TRACKPROXYMODEL_H
