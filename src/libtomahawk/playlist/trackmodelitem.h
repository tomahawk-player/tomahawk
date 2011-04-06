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

#ifndef PLITEM_H
#define PLITEM_H

#include <QHash>
#include <QVector>
#include <QPersistentModelIndex>
#include <QAbstractItemModel>

#include "query.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT TrackModelItem : public QObject
{
Q_OBJECT

public:
    virtual ~TrackModelItem();

    explicit TrackModelItem( TrackModelItem* parent = 0, QAbstractItemModel* model = 0 );
    explicit TrackModelItem( const QString& caption, TrackModelItem* parent = 0 );
    explicit TrackModelItem( const Tomahawk::query_ptr& query, TrackModelItem* parent = 0, int row = -1 );
    explicit TrackModelItem( const Tomahawk::plentry_ptr& entry, TrackModelItem* parent = 0, int row = -1 );

    const Tomahawk::plentry_ptr& entry() const;
    const Tomahawk::query_ptr& query() const;

    bool isPlaying() { return m_isPlaying; }
    void setIsPlaying( bool b ) { m_isPlaying = b; emit dataChanged(); }

    TrackModelItem* parent;
    QVector<TrackModelItem*> children;
    QHash<QString, TrackModelItem*> hash;
    QString caption;
    int childCount;
    QPersistentModelIndex index;
    QAbstractItemModel* model;
    bool toberemoved;

signals:
    void dataChanged();

private:
    void setupItem( const Tomahawk::query_ptr& query, TrackModelItem* parent, int row = -1 );

    Tomahawk::plentry_ptr m_entry;
    Tomahawk::query_ptr m_query;
    bool m_isPlaying;
};

#endif // PLITEM_H
