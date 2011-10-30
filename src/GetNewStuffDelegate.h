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

#ifndef GETNEWSTUFFDELEGATE_H
#define GETNEWSTUFFDELEGATE_H

#include <QStyledItemDelegate>


class
GetNewStuffDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit GetNewStuffDelegate( QObject* parent = 0 );
    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

protected:
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private:
    QPixmap m_defaultCover, m_onHoverStar, m_ratingStarPositive, m_ratingStarNegative;

    int m_widestTextWidth;
    int m_hoveringOver;
    QPair<int, int> m_hoveringItem;
    mutable QHash< QPair<int, int>, QRect > m_cachedButtonRects;
    mutable QHash< QPair<int, int>, QRect > m_cachedStarRects;
};

#endif // GETNEWSTUFFDELEGATE_H
