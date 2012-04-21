/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef ACLJOBITEM_H
#define ACLJOBITEM_H

#include <QStyledItemDelegate>

#include <jobview/JobStatusItem.h>

class QListView;

class AclJobDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit AclJobDelegate ( QObject* parent = 0 );
    virtual ~AclJobDelegate();

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual void setParent( QObject* parent );

private:
    mutable QHash< QPersistentModelIndex, int > m_cachedMultiLineHeights;
    QListView* m_parentView;
};


class AclJobItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit AclJobItem();
    ~AclJobItem();
    
    void done();

    virtual QString rightColumnText() const { return QString(); }
    virtual QString mainText() const { return QString(); }
    virtual QPixmap icon() const { return QPixmap(); }
    virtual QString type() const { return "acljob"; }

    virtual int concurrentJobLimit() const { return 3; }
    
    virtual bool hasCustomDelegate() { return true; }
    virtual void createDelegate( QObject* parent );
    virtual QStyledItemDelegate* customDelegate() { return m_delegate; }

private:
    QStyledItemDelegate* m_delegate;
};

#endif // ACLJOBITEMJOBITEM_H
