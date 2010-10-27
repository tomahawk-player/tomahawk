#include "sourcetreedelegate.h"

#include <QLineEdit>
#include <QAbstractItemView>


SourceTreeDelegate::SourceTreeDelegate( QAbstractItemView* parent )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
{
}


QWidget*
SourceTreeDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QLineEdit* editor = new QLineEdit( parent );
    editor->setFrame( false );
    editor->setObjectName( "playlistEditor" );

    return editor;
}


void
SourceTreeDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( editor->objectName() != "playlistEditor" )
        return;

    QRect r = option.rect;
    r.adjust( 20, -1, -8, 1 );

    editor->setGeometry( r );
}


void
SourceTreeDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const
{
     QLineEdit* le = static_cast<QLineEdit*>( editor );

     model->setData( index, le->text(), Qt::EditRole );
}


QSize
SourceTreeDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );
    return size;
}
