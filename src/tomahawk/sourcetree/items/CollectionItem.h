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


#ifndef COLLECTIONITEM_H
#define COLLECTIONITEM_H

#include "SourceTreeItem.h"
#include "libtomahawk/collection/Collection.h"

class CollectionItem : public SourceTreeItem
{
    Q_OBJECT
public:
    explicit CollectionItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::collection_ptr& collection );
    virtual ~CollectionItem();

    virtual QString text() const;
    virtual QIcon icon() const;
    virtual int peerSortValue() const;
    void setSortValue( int value );

    int trackCount() const;

public slots:
    virtual void activate();

private:
    int m_sortValue;
    QIcon m_icon;
    QString m_text;
    Tomahawk::collection_ptr m_collection;
};

#endif // COLLECTIONITEM_H
