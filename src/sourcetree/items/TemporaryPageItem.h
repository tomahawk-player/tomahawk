/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef TEMPORARYPAGEITEM_H
#define TEMPORARYPAGEITEM_H

#include "items/SourceTreeItem.h"
#include "ViewPage.h"

class QAction;

class TemporaryPageItem : public SourceTreeItem
{
    Q_OBJECT
public:
    TemporaryPageItem ( SourcesModel* model, SourceTreeItem* parent, Tomahawk::ViewPage* page, int sortValue );

    virtual QString text() const;
    virtual void activate();

    virtual QIcon icon() const;
    virtual int peerSortValue() const;
    virtual int IDValue() const;
    virtual QList< QAction* > customActions() const;

    Tomahawk::ViewPage* page() const;
    virtual bool isBeingPlayed() const;

public slots:
    void removeFromList();

signals:
    bool removed();

private slots:
    void linkActionTriggered( QAction* );

private:
    Tomahawk::ViewPage* m_page;
    QIcon m_icon;
    int m_sortValue;
    QList< QAction* > m_customActions;
};

Q_DECLARE_METATYPE( QAction* )

#endif // TEMPORARYPAGEITEM_H
