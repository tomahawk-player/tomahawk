/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QtGui/QSortFilterProxyModel>

#include "playlistinterface.h"
#include "playlist/trackmodel.h"

#include "dllmacro.h"

class DLLEXPORT TrackProxyModel : public QSortFilterProxyModel, public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    explicit TrackProxyModel ( QObject* parent = 0 );
    
    virtual TrackModel* sourceModel() const { return m_model; }
    virtual void setSourceTrackModel( TrackModel* sourceModel );
    virtual void setSourceModel( QAbstractItemModel* model );

    virtual QPersistentModelIndex currentIndex() const { return mapFromSource( m_model->currentItem() ); }
    virtual void setCurrentIndex( const QModelIndex& index ) { m_model->setCurrentItem( mapToSource( index ) ); }

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int unfilteredTrackCount() const { return sourceModel()->trackCount(); }
    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual void remove( const QModelIndex& index );
    virtual void remove( const QModelIndexList& indexes );
    virtual void remove( const QList< QPersistentModelIndex >& indexes );

    virtual Tomahawk::result_ptr currentItem() const;
    virtual Tomahawk::result_ptr siblingItem( int itemsAway );
    virtual Tomahawk::result_ptr siblingItem( int itemsAway, bool readOnly );
    virtual bool hasNextItem();

    virtual QString filter() const { return filterRegExp().pattern(); }
    virtual void setFilter( const QString& pattern );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }

    bool showOfflineResults() const { return m_showOfflineResults; }
    void setShowOfflineResults( bool b ) { m_showOfflineResults = b; }

    TrackModelItem* itemFromIndex( const QModelIndex& index ) const { return sourceModel()->itemFromIndex( index ); }

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

    void filterChanged( const QString& filter );

    void nextTrackReady();

public slots:
    virtual void setRepeatMode( RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

private:
    TrackModel* m_model;
    RepeatMode m_repeatMode;
    bool m_shuffled;
    bool m_showOfflineResults;
};

#endif // TRACKPROXYMODEL_H
