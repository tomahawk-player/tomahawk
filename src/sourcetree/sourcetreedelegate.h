#ifndef SOURCETREEDELEGATE_H
#define SOURCETREEDELEGATE_H

#include <QDebug>
#include <QStyledItemDelegate>

class SourceTreeDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    SourceTreeDelegate( QAbstractItemView* parent = 0 );

protected:
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const;

private:
    QAbstractItemView* m_view;
};

#endif // SOURCETREEDELEGATE_H
