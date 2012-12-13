/*
 *  Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CATEGORY_ITEM_H
#define CATEGORY_ITEM_H

#include "SourceTreeItem.h"

class CategoryAddItem : public SourceTreeItem
{
    Q_OBJECT
public:
    CategoryAddItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType type );
    ~CategoryAddItem();

    virtual Qt::ItemFlags flags() const;
    virtual QString text() const;
    virtual void activate();
    virtual QIcon icon() const;
    virtual int peerSortValue() const;

    virtual bool willAcceptDrag(const QMimeData* data) const;
    virtual DropTypes supportedDropTypes(const QMimeData* data) const;
    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action);

private slots:
    void parsedDroppedTracks( const QList< Tomahawk::query_ptr >& tracks );

    // Do the rename only after the revision is loaded
    void playlistToRenameLoaded();

private:
    SourcesModel::CategoryType m_categoryType;
};

class CategoryItem : public SourceTreeItem
{
    Q_OBJECT
public:
    CategoryItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType category, bool showAddItem );

    virtual QString text() const;
    virtual void activate();
    virtual int peerSortValue() const;
    virtual Qt::ItemFlags flags() const;

    // inserts an item at the end, but before the category add item
    void insertItem( SourceTreeItem* item );
    void insertItems( QList< SourceTreeItem* > item );

    SourcesModel::CategoryType categoryType();

private:
    SourcesModel::CategoryType m_category;
    CategoryAddItem* m_addItem;
    bool m_showAdd;
};


#endif
