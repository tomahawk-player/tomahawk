/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef SCRIPTCOLLECTIONITEM_H
#define SCRIPTCOLLECTIONITEM_H

#include "SourceTreeItem.h"

class ScriptCollectionItem : public SourceTreeItem
{
    Q_OBJECT
public:
    explicit ScriptCollectionItem( SourcesModel* model,
                                   SourceTreeItem* parent,
                                   const Tomahawk::collection_ptr& collection );

    virtual ~ScriptCollectionItem();
    virtual void activate();
    virtual QString text() const;
    virtual QString tooltip() const;
    virtual QIcon icon() const;
    virtual bool isBeingPlayed() const;

    virtual Tomahawk::collection_ptr collection() const;

private:
    QIcon m_icon;
    QString m_text;

    Tomahawk::collection_ptr m_collection;
    Tomahawk::ViewPage* m_page;
};

#endif // SCRIPTCOLLECTIONITEM_H
