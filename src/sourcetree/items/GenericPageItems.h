/*
 *    Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef GENERIC_PAGE_ITEM_H
#define GENERIC_PAGE_ITEM_H

#include "SourceTreeItem.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"

// generic item that has some name, some text, and calls a certain slot when activated. badabing!
class GenericPageItem : public SourceTreeItem
{
    Q_OBJECT
public:
    // takes 2 function pointers: show: called when wanting to show the desired view page. get: called to get the view page from ViewManager if it exists
    GenericPageItem( SourcesModel* model, SourceTreeItem* parent, const QString& text, const QIcon& icon, boost::function<Tomahawk::ViewPage*()> show, boost::function<Tomahawk::ViewPage*()> get );
    virtual ~GenericPageItem();

    virtual QString text() const;
    virtual void activate();
    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual QIcon icon() const;
    virtual int peerSortValue() const; // How to sort relative to peers in the tree.
    virtual bool isBeingPlayed() const;

    void setText( const QString& text );
    void setSortValue( int value );

signals:
    void activated();

private:
    QIcon m_icon;
    QString m_text;
    int m_sortValue;
    boost::function< Tomahawk::ViewPage*() > m_show;
    boost::function< Tomahawk::ViewPage*() > m_get;
};

#endif
