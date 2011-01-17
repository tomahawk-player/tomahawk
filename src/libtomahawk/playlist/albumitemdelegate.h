#ifndef ALBUMITEMDELEGATE_H
#define ALBUMITEMDELEGATE_H

#include <QStyledItemDelegate>

#include "dllmacro.h"

class AlbumProxyModel;

class DLLEXPORT AlbumItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    AlbumItemDelegate( QAbstractItemView* parent = 0, AlbumProxyModel* proxy = 0 );

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

//    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private:
    QAbstractItemView* m_view;
    AlbumProxyModel* m_model;
};

#endif // ALBUMITEMDELEGATE_H
