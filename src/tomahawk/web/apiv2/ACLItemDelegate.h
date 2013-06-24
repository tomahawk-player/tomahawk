/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef TOMAHAWK_APIV2_ACLITEMDELEGATE_H
#define TOMAHAWK_APIV2_ACLITEMDELEGATE_H

#include <QStyledItemDelegate>

#include "web/Api2User.h"

namespace Tomahawk {
namespace APIv2 {

class ACLItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ACLItemDelegate ( QObject* parent = 0 );
    virtual ~ACLItemDelegate();

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual void emitSizeHintChanged( const QModelIndex &index );

signals:
    void update( const QModelIndex& idx );
    void aclResult( Api2User::ACLDecision aclDecision );

protected:
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private:
    void drawRoundedButton( QPainter* painter, const QRect& btnRect, bool red = false ) const;

    QPoint m_savedHoverPos;
    mutable QRect m_savedAcceptRect;
};

} // namespace APIv2
} // namespace Tomahawk

#endif // TOMAHAWK_APIV2_ACLITEMDELEGATE_H
