/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Christopher Reichert <creichert07@gmail.com>
 *   Copyright 2012-2016, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef LOVED_TRACKS_ITEM_H
#define LOVED_TRACKS_ITEM_H

#include "SourceTreeItem.h"

#include <QString>
#include <QIcon>


class LovedTracksItem : public SourceTreeItem
{
    Q_OBJECT

public:
    LovedTracksItem( SourcesModel* model, SourceTreeItem* parent );
    virtual ~LovedTracksItem();

    virtual QString text() const;
    virtual QIcon icon() const;
    virtual int peerSortValue() const;
    virtual void activate();

    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual DropTypes supportedDropTypes( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action );

    void setSortValue( int value );

    virtual bool isBeingPlayed() const;

private slots:
    void loveDroppedTracks( QList< Tomahawk::query_ptr > qrys );

private:
    Tomahawk::ViewPage* m_lovedTracksPage;
    int m_sortValue;
};

#endif
