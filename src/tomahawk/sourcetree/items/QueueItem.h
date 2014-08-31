/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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


#ifndef QUEUEITEM_H
#define QUEUEITEM_H

#include "SourceTreeItem.h"

class QueueItem : public SourceTreeItem
{
    Q_OBJECT
public:
    explicit QueueItem( SourcesModel* model, SourceTreeItem* parent );
    virtual ~QueueItem();

    virtual QString text() const;
    virtual QIcon icon() const;
    virtual int peerSortValue() const;
    void setSortValue( int value );

    int unlistenedCount() const;

    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual DropTypes supportedDropTypes( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action );

public slots:
    virtual void activate();

private slots:
    void parsedDroppedTracks( const QList<Tomahawk::query_ptr>& tracks );

private:
    int m_sortValue;
    QIcon m_icon;
    QString m_text;
};

#endif // QUEUEITEM_H
