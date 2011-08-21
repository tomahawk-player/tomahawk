#ifndef SOURCEDELEGATE_H
#define SOURCEDELEGATE_H

#include "sourcetreeview.h"
#include "items/sourcetreeitem.h"

#include <QStyledItemDelegate>

class SourceDelegate : public QStyledItemDelegate
{
public:
    SourceDelegate( QAbstractItemView* parent = 0 ) : QStyledItemDelegate( parent ), m_parent( parent ) {}

    void setDropHoverIndex( const QModelIndex &index, const QMimeData *mimeData ) { m_dropHoverIndex = index; m_dropMimeData = const_cast< QMimeData* >( mimeData ); }

    SourceTreeItem::DropType hoveredDropType() const;

protected:
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual int dropTypeCount( SourceTreeItem* item ) const;
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private:
    QAbstractItemView* m_parent;
    mutable int m_iconHeight;
    QModelIndex m_dropHoverIndex;
    QMimeData *m_dropMimeData;
    mutable SourceTreeItem::DropType m_hoveredDropType; // Hack to keep easily track of the current highlighted DropType in paint()
};

#endif // SOURCEDELEGATE_H
