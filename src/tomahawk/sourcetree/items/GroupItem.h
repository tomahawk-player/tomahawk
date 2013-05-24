/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef GROUP_ITEM_H
#define GROUP_ITEM_H

#include "SourceTreeItem.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"

class GroupItem : public SourceTreeItem
{
    Q_OBJECT
public:
    // takes 2 function pointers: show: called when wanting to show the desired view page. get: called to get the view page from ViewManager if it exists
    GroupItem( SourcesModel* model, SourceTreeItem* parent, const QString& text, int peerSortValue = 0 );
    virtual ~GroupItem();

    virtual QString text() const;
    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual QIcon icon() const;
    virtual bool isBeingPlayed() const;

    void checkExpandedState();
    void setDefaultExpanded( bool b );

public slots:
    virtual void activate();

signals:
    void activated();

private slots:
    void requestExpanding();

private:
    QString m_text;
    bool m_defaultExpanded;
};

#endif
