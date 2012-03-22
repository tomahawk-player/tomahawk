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

#include <QtGui/QSortFilterProxyModel>

#include "playlistinterface.h"
#include "playlist/trackmodel.h"

#include "dllmacro.h"

class DLLEXPORT TrackProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    explicit TrackProxyModel ( QObject* parent = 0 );
    virtual ~TrackProxyModel() {}

    virtual TrackModel* sourceModel() const { return m_model; }
    virtual void setSourceTrackModel( TrackModel* sourceModel );
    virtual void setSourceModel( QAbstractItemModel* model );

    virtual QPersistentModelIndex currentIndex() const { return mapFromSource( m_model->currentItem() ); }
    virtual void setCurrentIndex( const QModelIndex& index ) { m_model->setCurrentItem( mapToSource( index ) ); }

    virtual void remove( const QModelIndex& index );
    virtual void remove( const QModelIndexList& indexes );
    virtual void remove( const QList< QPersistentModelIndex >& indexes );

    virtual bool showOfflineResults() const { return m_showOfflineResults; }
    virtual void setShowOfflineResults( bool b ) { m_showOfflineResults = b; }

    virtual void emitFilterChanged( const QString &pattern ) { emit filterChanged( pattern ); }

    virtual TrackModelItem* itemFromIndex( const QModelIndex& index ) const { return sourceModel()->itemFromIndex( index ); }

    virtual Tomahawk::playlistinterface_ptr playlistInterface();

signals:
    void filterChanged( const QString& filter );

protected:
    virtual bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;
    virtual bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

    TrackModel* m_model;
    bool m_showOfflineResults;
    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

#endif // TRACKPROXYMODEL_H
