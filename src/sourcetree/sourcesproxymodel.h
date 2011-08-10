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

#ifndef SOURCESPROXYMODEL_H
#define SOURCESPROXYMODEL_H

#include <QSortFilterProxyModel>

class SourcesModel;

class SourcesProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    explicit SourcesProxyModel( SourcesModel* model, QObject* parent = 0 );

public slots:
    void showOfflineSources( bool offlineSourcesShown );

    void selectRequested( const QModelIndex& );
    void expandRequested( const QModelIndex& );

signals:
    void selectRequest( const QModelIndex& idx );
    void expandRequest( const QModelIndex& idx );

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;

private:
    SourcesModel* m_model;

    bool m_filtered;
};

#endif // SOURCESPROXYMODEL_H
