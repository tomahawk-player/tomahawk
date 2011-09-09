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

#ifndef TREEMODELITEM_H
#define TREEMODELITEM_H

#include <QAbstractItemModel>
#include <QHash>
#include <QPersistentModelIndex>
#include <QPixmap>

#include "result.h"

#include "dllmacro.h"

class DLLEXPORT TreeModelItem : public QObject
{
Q_OBJECT

public:
    ~TreeModelItem();

    explicit TreeModelItem( TreeModelItem* parent = 0, QAbstractItemModel* model = 0 );
    explicit TreeModelItem( const Tomahawk::artist_ptr& artist, TreeModelItem* parent = 0, int row = -1 );
    explicit TreeModelItem( const Tomahawk::album_ptr& album, TreeModelItem* parent = 0, int row = -1 );
    explicit TreeModelItem( const Tomahawk::result_ptr& result, TreeModelItem* parent = 0, int row = -1 );
    explicit TreeModelItem( const Tomahawk::query_ptr& query, TreeModelItem* parent = 0, int row = -1 );

    const Tomahawk::artist_ptr& artist() const { return m_artist; };
    const Tomahawk::album_ptr& album() const { return m_album; };
    const Tomahawk::result_ptr& result() const { return m_result; };
    const Tomahawk::query_ptr& query() const { return m_query; };

    bool isPlaying() { return m_isPlaying; }
    void setIsPlaying( bool b ) { m_isPlaying = b; emit dataChanged(); }

    void setCover( const QPixmap& cover ) { this->cover = cover; emit dataChanged(); }

    QString name() const;
    QString artistName() const;
    QString albumName() const;

    TreeModelItem* parent;
    QList<TreeModelItem*> children;
    QHash<QString, TreeModelItem*> hash;
    int childCount;
    QPersistentModelIndex index;
    QAbstractItemModel* model;
    QPixmap cover;

    bool toberemoved;
    bool fetchingMore;

signals:
    void dataChanged();

private slots:
    void onResultsChanged();

private:
    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
    Tomahawk::result_ptr m_result;
    Tomahawk::query_ptr m_query;

    bool m_isPlaying;
};

#endif // TREEMODELITEM_H
