#include "sourcedelegate.h"

#include "items/sourcetreeitem.h"
#include "items/collectionitem.h"
#include "items/playlistitems.h"

#include "utils/tomahawkutils.h"

#include <QApplication>
#include <QPainter>

#define TREEVIEW_INDENT_ADD -7

QSize
SourceDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    SourceTreeItem *item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();

    if ( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::Collection )
        return QSize( option.rect.width(), 44 );
    else if ( index == m_dropHoverIndex )
    {
        QSize originalSize = QStyledItemDelegate::sizeHint( option, index );
        return originalSize + QSize( 0, originalSize.height() * dropTypeCount( item ) );
    }
    else
        return QStyledItemDelegate::sizeHint( option, index );
}


void
SourceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem o = option;

#ifdef Q_WS_MAC
    QFont savedFont = painter->font();
    QFont smaller = savedFont;
    smaller.setPointSize( smaller.pointSize() - 2 );
    painter->setFont( smaller );
    o.font = smaller;
#endif

    if ( ( option.state & QStyle::State_Enabled ) == QStyle::State_Enabled )
    {
        o.state = QStyle::State_Enabled;

        if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
        }
    }

    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    Q_ASSERT( item );

    QStyleOptionViewItemV4 o3 = option;
    if ( type != SourcesModel::Collection && type != SourcesModel::Category )
        o3.rect.setX( 0 );

    QApplication::style()->drawControl( QStyle::CE_ItemViewItem, &o3, painter );

    if ( type == SourcesModel::Collection )
    {
        painter->save();

        QFont normal = painter->font();
        QFont bold = painter->font();
        bold.setBold( true );

        CollectionItem* colItem = qobject_cast< CollectionItem* >( item );
        Q_ASSERT( colItem );
        bool status = !( !colItem || colItem->source().isNull() || !colItem->source()->isOnline() );

        QString tracks;
        QString name = index.data().toString();
        int figWidth = 0;

        if ( status && colItem && !colItem->source().isNull() )
        {
            tracks = QString::number( colItem->source()->trackCount() );
            figWidth = painter->fontMetrics().width( tracks );
            name = colItem->source()->friendlyName();
        }

        QRect iconRect = option.rect.adjusted( 4, 6, -option.rect.width() + option.rect.height() - 12 + 4, -6 );

        QPixmap avatar = colItem->icon().pixmap( iconRect.size() );
        painter->drawPixmap( iconRect, avatar.scaledToHeight( iconRect.height(), Qt::SmoothTransformation ) );

        if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            painter->setPen( o.palette.color( QPalette::HighlightedText ) );
        }

        QRect textRect = option.rect.adjusted( iconRect.width() + 8, 6, -figWidth - 24, 0 );
        if ( status || colItem->source().isNull() )
            painter->setFont( bold );
        QString text = painter->fontMetrics().elidedText( name, Qt::ElideRight, textRect.width() );
        painter->drawText( textRect, text );

        QString desc = status ? colItem->source()->textStatus() : tr( "Offline" );
        if ( colItem->source().isNull() )
            desc = tr( "All available tracks" );
        if ( status && desc.isEmpty() && !colItem->source()->currentTrack().isNull() )
            desc = colItem->source()->currentTrack()->artist() + " - " + colItem->source()->currentTrack()->track();
        if ( desc.isEmpty() )
            desc = tr( "Online" );

        textRect = option.rect.adjusted( iconRect.width() + 8, painter->fontMetrics().height() + 6, -figWidth - 24, -4 );
        painter->setFont( normal );
        text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() );
        QTextOption to( Qt::AlignBottom );
        painter->drawText( textRect, text, to );

        if ( status )
        {
            painter->setRenderHint( QPainter::Antialiasing );

            QRect figRect = o.rect.adjusted( o.rect.width() - figWidth - 8, 0, -13, -o.rect.height() + 16 );
            int hd = ( option.rect.height() - figRect.height() ) / 2;
            figRect.adjust( 0, hd, 0, hd );
#ifdef Q_OS_WIN
            figRect.adjust( -3, 0, 3, 0 );
#endif
            painter->setFont( bold );

            QColor figColor( 167, 183, 211 );
            painter->setPen( figColor );
            painter->setBrush( figColor );

            TomahawkUtils::drawBackgroundAndNumbers( painter, tracks, figRect );
        }

        painter->restore();
    }
    else if ( type == SourcesModel::StaticPlaylist )
    {
        painter->save();

        QFont normal = painter->font();
        QFont bold = painter->font();
        bold.setBold( true );

        PlaylistItem* plItem = qobject_cast< PlaylistItem* >( item );
        Q_ASSERT( plItem );

//        QString tracks;
        QString name = index.data().toString();
//        int figWidth = 0;

        if ( plItem && !plItem->playlist().isNull() )
        {
//            tracks = QString::number( plItem->source()->trackCount() );
//            figWidth = painter->fontMetrics().width( tracks );
            name = plItem->playlist()->title();
        }

        int height = option.rect.height();
        if ( index == m_dropHoverIndex )
            height /= ( dropTypeCount( item ) + 1 );

        QRect iconRect = option.rect.adjusted( 4, 1, -option.rect.width() + height - 2 + 4, -option.rect.height() + height -1 );

        QPixmap avatar = index.data( Qt::DecorationRole ).value< QIcon >().pixmap( iconRect.width(), iconRect.height() );
        painter->drawPixmap( iconRect, avatar.scaledToHeight( iconRect.height(), Qt::SmoothTransformation ) );

        if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            painter->setPen( o.palette.color( QPalette::HighlightedText ) );
        }

        QRect textRect = option.rect.adjusted( iconRect.width() + 8, 2, /*-figWidth - 24*/ 0, 0 );
        QString text = painter->fontMetrics().elidedText( name, Qt::ElideRight, textRect.width() );
        painter->drawText( textRect, text );

        if ( index == m_dropHoverIndex )
        {
            QPoint cursorPos = m_parent->mapFromGlobal( QCursor::pos() );
            qDebug() << "cursorpos is" << cursorPos;

            int hoveredDropTypeIndex = ( cursorPos.y() - o.rect.y() ) / height;
            int verticalOffset = height * hoveredDropTypeIndex;
            QRect selectionRect = o.rect.adjusted( 0, verticalOffset, 0, -o.rect.height() + height + verticalOffset );
            painter->drawRoundedRect( selectionRect, 5, 5 );

            int count = 1;
            if ( item->supportedDropTypes().testFlag( SourceTreeItem::DropTypeAllItems ) )
            {
                text = "All items";
                textRect = option.rect.adjusted( iconRect.width() + 8, 2 + ( count * height ), 0, 0 );
                painter->drawText( textRect, text );
                if ( count == hoveredDropTypeIndex )
                    m_hoveredDropType = SourceTreeItem::DropTypeAllItems;
                count++;
            }
            if ( item->supportedDropTypes().testFlag( SourceTreeItem::DropTypeLocalItems ) )
            {
                text = "Local items";
                textRect = option.rect.adjusted( iconRect.width() + 8, 2 + ( count * height ), 0, 0 );
                painter->drawText( textRect, text );
                if ( count == hoveredDropTypeIndex )
                    m_hoveredDropType = SourceTreeItem::DropTypeLocalItems;
                count++;
            }
            if ( item->supportedDropTypes().testFlag( SourceTreeItem::DropTypeTop10 ) )
            {
                text = "Top 10";
                textRect = option.rect.adjusted( iconRect.width() + 8, 2 + ( count * height ), 0, 0 );
                painter->drawText( textRect, text );
                if ( count == hoveredDropTypeIndex )
                    m_hoveredDropType = SourceTreeItem::DropTypeTop10;
                count++;
            }

            qDebug() << "***************+ verticaloffset:" << hoveredDropTypeIndex << count;
        }

//        QString desc = status ? colItem->source()->textStatus() : tr( "Offline" );
//        if ( colItem->source().isNull() )
//            desc = tr( "All available tracks" );
//        if ( status && desc.isEmpty() && !colItem->source()->currentTrack().isNull() )
//            desc = colItem->source()->currentTrack()->artist() + " - " + colItem->source()->currentTrack()->track();
//        if ( desc.isEmpty() )
//            desc = tr( "Online" );

//        textRect = option.rect.adjusted( iconRect.width() + 8, painter->fontMetrics().height() + 6, -figWidth - 24, -4 );
//        painter->setFont( normal );
//        text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() );
//        QTextOption to( Qt::AlignBottom );
//        painter->drawText( textRect, text, to );

//        if ( status )
//        {
//            painter->setRenderHint( QPainter::Antialiasing );

//            QRect figRect = o.rect.adjusted( o.rect.width() - figWidth - 8, 0, -13, -o.rect.height() + 16 );
//            int hd = ( option.rect.height() - figRect.height() ) / 2;
//            figRect.adjust( 0, hd, 0, hd );
//#ifdef Q_OS_WIN
//            figRect.adjust( -3, 0, 3, 0 );
//#endif
//            painter->setFont( bold );

//            QColor figColor( 167, 183, 211 );
//            painter->setPen( figColor );
//            painter->setBrush( figColor );

//            TomahawkUtils::drawBackgroundAndNumbers( painter, tracks, figRect );
//        }

        painter->restore();

    }
    else
    {
        QStyledItemDelegate::paint( painter, o, index );
        /*QStyleOptionViewItemV4 opt = o;
        initStyleOption( &opt, index );

        // shrink the indentations. count how indented this item is and remove it
        int indentMult = 0;
        QModelIndex counter = index;
        while ( counter.parent().isValid() )
        {
            indentMult++;
            counter = counter.parent();
        }
        int realX = opt.rect.x() + indentMult * TREEVIEW_INDENT_ADD;

        opt.rect.setX( realX );
        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, widget ); */
    }

#ifdef Q_WS_MAC
    painter->setFont( savedFont );
#endif
}

void
SourceDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::StaticPlaylist )
        editor->setGeometry( option.rect.adjusted( 20, 0, 0, 0 ) );
    else
        QStyledItemDelegate::updateEditorGeometry( editor, option, index );

    editor->setGeometry( editor->geometry().adjusted( 2*TREEVIEW_INDENT_ADD, 0, 0, 0 ) );
}

int
SourceDelegate::dropTypeCount( SourceTreeItem* item ) const
{
    int menuCount = 0;
    if ( item->supportedDropTypes().testFlag( SourceTreeItem::DropTypeAllItems ) )
        menuCount++;

    if ( item->supportedDropTypes().testFlag( SourceTreeItem::DropTypeLocalItems ) )
        menuCount++;

    if ( item->supportedDropTypes().testFlag( SourceTreeItem::DropTypeTop10 ) )
        menuCount++;

    return menuCount;
}

SourceTreeItem::DropType
SourceDelegate::hoveredDropType() const
{
    return m_hoveredDropType;
}
