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

#include "AclRegistry.h"
#include "DllMacro.h"
#include "jobview/JobStatusItem.h"

#include <QStyledItemDelegate>

class QListView;

class DLLEXPORT ACLJobDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ACLJobDelegate ( QObject* parent = 0 );
    virtual ~ACLJobDelegate();

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual void emitSizeHintChanged( const QModelIndex &index );

signals:
    void update( const QModelIndex& idx );
    void aclResult( ACLRegistry::ACL result );

protected:
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private:
    void drawRoundedButton( QPainter* painter, const QRect& btnRect, bool red = false ) const;

    QPoint m_savedHoverPos;
    mutable QRect m_savedAcceptRect;
    mutable QRect m_savedDenyRect;
};


class DLLEXPORT ACLJobItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit ACLJobItem( ACLRegistry::User user, const QString &username );
    virtual ~ACLJobItem();

    virtual int weight() const { return 99; }
    
    virtual QString rightColumnText() const { return QString(); }
    virtual QString mainText() const { return QString(); }
    virtual QPixmap icon() const { return QPixmap(); }
    virtual QString type() const { return "acljob"; }

    virtual int concurrentJobLimit() const { return 3; }

    virtual bool hasCustomDelegate() const { return true; }
    virtual void createDelegate( QObject* parent = 0 );
    virtual QStyledItemDelegate* customDelegate() const { return m_delegate; }

    virtual ACLRegistry::User user() const { return m_user; }
    virtual const QString& username() const { return m_username; }
    
signals:
    void userDecision( ACLRegistry::User user );

public slots:
    void aclResult( ACLRegistry::ACL result );
    
private:
    QStyledItemDelegate* m_delegate;
    ACLRegistry::User m_user;
    const QString m_username;
};

#endif // ACLJOBITEM_H
