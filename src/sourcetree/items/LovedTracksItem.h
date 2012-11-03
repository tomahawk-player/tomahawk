/*
 *    Copyright 2012, Christopher Reichert <creichert07@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
    virtual QIcon icon() const { return QIcon( RESPATH "images/loved_playlist.png" ); }
    virtual int peerSortValue() const { return m_sortValue; }
    virtual void activate();

    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual DropTypes supportedDropTypes( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action );

    void setSortValue( int value ) { m_sortValue = value; }

private slots:
    void loveDroppedTracks( QList< Tomahawk::query_ptr > qrys );

private:
    Tomahawk::ViewPage* m_lovedTracksPage;
    int m_sortValue;
};

#endif
