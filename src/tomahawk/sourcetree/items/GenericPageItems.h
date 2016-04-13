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

#ifndef GENERIC_PAGE_ITEM_H
#define GENERIC_PAGE_ITEM_H

#include "SourceTreeItem.h"

#include <functional>

// generic item that has some name, some text, and calls a certain slot when activated. badabing!
class GenericPageItem : public SourceTreeItem
{
    Q_OBJECT
public:
    // takes 2 function pointers: show: called when wanting to show the desired view page. get: called to get the view page from ViewManager if it exists
    GenericPageItem( SourcesModel* model, SourceTreeItem* parent,
                     const QString& text, const QIcon& icon,
                     std::function<Tomahawk::ViewPage*()> show,
                     std::function<Tomahawk::ViewPage*()> get );
    virtual ~GenericPageItem();

    virtual QString text() const;
    virtual void activate();
    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData*, Qt::DropAction );
    virtual QIcon icon() const;
    virtual int peerSortValue() const; // How to sort relative to peers in the tree.
    virtual bool isBeingPlayed() const;

    void setRemovable( bool removable );
    void setDeletable( bool deletable );
    void setText( const QString& text );
    void setSortValue( int value );

public slots:
    void removeFromList();

signals:
    void activated();

private:
    QIcon m_icon;
    QString m_text;
    int m_sortValue;
    std::function< Tomahawk::ViewPage*() > m_show;
    std::function< Tomahawk::ViewPage*() > m_get;
};

#endif
