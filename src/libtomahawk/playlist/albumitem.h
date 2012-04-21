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

#ifndef ALBUMITEM_H
#define ALBUMITEM_H

#include <QAbstractItemModel>
#include <QHash>
#include <QPersistentModelIndex>
#include <QPixmap>

#include "Artist.h"
#include "Album.h"

#include "DllMacro.h"

class DLLEXPORT AlbumItem : public QObject
{
Q_OBJECT

public:
    ~AlbumItem();

    explicit AlbumItem( AlbumItem* parent = 0, QAbstractItemModel* model = 0 );
    explicit AlbumItem( const Tomahawk::artist_ptr& artist, AlbumItem* parent = 0, int row = -1 );
    explicit AlbumItem( const Tomahawk::album_ptr& album, AlbumItem* parent = 0, int row = -1 );

    const Tomahawk::artist_ptr& artist() const { return m_artist; }
    const Tomahawk::album_ptr& album() const { return m_album; }

    AlbumItem* parent;
    QList<AlbumItem*> children;
    QHash<QString, AlbumItem*> hash;
    int childCount;
    QPersistentModelIndex index;
    QAbstractItemModel* model;
    bool toberemoved;

signals:
    void dataChanged();

private:
    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
};

#endif // ALBUMITEM_H
