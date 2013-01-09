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

class DLLEXPORT PlayableProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    enum PlayableItemStyle
    { Detailed = 0, Short = 1, ShortWithAvatars = 2, Large = 3, Collection = 4 };

    enum PlayableProxyModelRole
    { StyleRole = Qt::UserRole + 1, TypeRole };

    explicit PlayableProxyModel ( QObject* parent = 0 );
    virtual ~PlayableProxyModel() {}

    virtual QString guid() const;

    virtual PlayableModel* sourceModel() const { return m_model; }
    virtual void setSourcePlayableModel( PlayableModel* sourceModel );
    virtual void setSourceModel( QAbstractItemModel* model );

    virtual bool isLoading() const;

    PlayableProxyModel::PlayableItemStyle style() const { return m_style; }
    void setStyle( PlayableProxyModel::PlayableItemStyle style ) { m_style = style; }

    virtual QPersistentModelIndex currentIndex() const { return mapFromSource( m_model->currentItem() ); }
    virtual void setCurrentIndex( const QModelIndex& index );

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QModelIndexList& indexes );
    virtual void removeIndexes( const QList< QPersistentModelIndex >& indexes );

    virtual bool showOfflineResults() const { return m_showOfflineResults; }
    virtual void setShowOfflineResults( bool b );

    virtual bool hideDupeItems() const { return m_hideDupeItems; }
    virtual void setHideDupeItems( bool b );

    virtual int maxVisibleItems() const { return m_maxVisibleItems; }
    virtual void setMaxVisibleItems( int items );

    virtual PlayableItem* itemFromIndex( const QModelIndex& index ) const { return sourceModel()->itemFromIndex( index ); }
    virtual PlayableItem* itemFromQuery( const Tomahawk::query_ptr& query ) const { return sourceModel()->itemFromQuery( query ); }
    virtual PlayableItem* itemFromResult( const Tomahawk::result_ptr& result ) const { return sourceModel()->itemFromResult( result ); }

    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;
    void setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface );

    QList< double > columnWeights() const;

    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual void setFilter( const QString& pattern );
    virtual void updateDetailedInfo( const QModelIndex& index );

signals:
    void filterChanged( const QString& filter );

    void filteringStarted();
    void filteringFinished();

    void loadingStarted();
    void loadingFinished();

    void indexPlayable( const QModelIndex& index );
    void indexResolved( const QModelIndex& index );
    void currentIndexChanged();

    void itemCountChanged( unsigned int items );

protected:
    virtual bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;
    virtual bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

    Tomahawk::playlistinterface_ptr m_playlistInterface;

private slots:
    void onIndexPlayable( const QModelIndex& index );
    void onIndexResolved( const QModelIndex& index );

private:
    virtual bool lessThan( int column, const Tomahawk::query_ptr& left, const Tomahawk::query_ptr& right ) const;

    PlayableModel* m_model;

    bool m_showOfflineResults;
    bool m_hideDupeItems;
    int m_maxVisibleItems;

    QHash< PlayableItemStyle, QList<PlayableModel::Columns> > m_headerStyle;
    PlayableItemStyle m_style;
};

#endif // TRACKPROXYMODEL_H
