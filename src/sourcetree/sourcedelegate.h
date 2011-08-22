#ifndef SOURCEDELEGATE_H
#define SOURCEDELEGATE_H

#include "sourcetreeview.h"
#include "items/sourcetreeitem.h"

#include <QStyledItemDelegate>
#include <QPropertyAnimation>

class AnimationHelper;

class SourceDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SourceDelegate( QAbstractItemView* parent = 0 );

    void hovered( const QModelIndex &index, const QMimeData *mimeData );
    void dragLeaveEvent();

    SourceTreeItem::DropType hoveredDropType() const;

protected:
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual int dropTypeCount( SourceTreeItem* item ) const;
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private slots:
    void animationFinished( const QModelIndex& );
private:
    QAbstractItemView* m_parent;
    mutable int m_iconHeight;
    QModelIndex m_dropHoverIndex;
    QModelIndex m_newDropHoverIndex;
    QMimeData *m_dropMimeData;
    mutable SourceTreeItem::DropType m_hoveredDropType; // Hack to keep easily track of the current highlighted DropType in paint()
    QMap< QModelIndex, AnimationHelper* > m_expandedMap;

    QMap< int, SourceTreeItem::DropType > m_dropTypeMap;
    QMap< int, QString > m_dropTypeTextMap;
    QMap< int, QPixmap > m_dropTypeImageMap;

};

#endif // SOURCEDELEGATE_H
