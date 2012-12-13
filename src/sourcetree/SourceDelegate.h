/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef SOURCEDELEGATE_H
#define SOURCEDELEGATE_H

#include "SourceTreeView.h"
#include "items/SourceTreeItem.h"

#include <QStyledItemDelegate>
#include <QPropertyAnimation>

class AnimationHelper;

class SourceDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SourceDelegate( QAbstractItemView* parent = 0 );
    ~SourceDelegate();

    void hovered( const QModelIndex& index, const QMimeData* mimeData );
    void dragLeaveEvent();

    SourceTreeItem::DropType hoveredDropType() const;

signals:
    void clicked( const QModelIndex& idx );
    void doubleClicked( const QModelIndex& idx );
    void latchOn( const Tomahawk::source_ptr& idx );
    void latchOff( const Tomahawk::source_ptr& idx );
    void toggleRealtimeLatch( const Tomahawk::source_ptr& idx, bool realtime );

protected:
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual int dropTypeCount( SourceTreeItem* item ) const;
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private slots:
    void animationFinished( const QModelIndex& );

private:
    void paintDecorations( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void paintCollection( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void paintCategory( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void paintGroup( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    QAbstractItemView* m_parent;
    mutable int m_iconHeight;
    QModelIndex m_dropHoverIndex;
    QModelIndex m_newDropHoverIndex;
    QMimeData* m_dropMimeData;
    mutable SourceTreeItem::DropType m_hoveredDropType; // Hack to keep easily track of the current highlighted DropType in paint()
    QMap< QModelIndex, AnimationHelper* > m_expandedMap;
    qint64 m_lastClicked;
    QMap< int, SourceTreeItem::DropType > m_dropTypeMap;
    QMap< int, QString > m_dropTypeTextMap;

    mutable QPersistentModelIndex m_trackHovered;
    mutable QHash< QPersistentModelIndex, QRect > m_trackRects;
    mutable QHash< QPersistentModelIndex, QRect > m_headphoneRects;
    mutable QHash< QPersistentModelIndex, QRect > m_lockRects;

    mutable QLinearGradient m_gradient;
};

#endif // SOURCEDELEGATE_H
