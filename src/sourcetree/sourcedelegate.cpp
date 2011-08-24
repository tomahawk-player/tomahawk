#include "sourcedelegate.h"

#include "items/sourcetreeitem.h"
#include "items/collectionitem.h"
#include "items/playlistitems.h"
#include "items/categoryitems.h"

#include "utils/tomahawkutils.h"
#include "items/temporarypageitem.h"
#include "animationhelper.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#define TREEVIEW_INDENT_ADD -7

SourceDelegate::SourceDelegate( QAbstractItemView* parent )
    : QStyledItemDelegate( parent )
    , m_parent( parent )
{
    m_dropTypeMap.insert( 0, SourceTreeItem::DropTypeThisTrack );
    m_dropTypeMap.insert( 1, SourceTreeItem::DropTypeThisAlbum );
    m_dropTypeMap.insert( 2, SourceTreeItem::DropTypeAllFromArtist );
    m_dropTypeMap.insert( 3, SourceTreeItem::DropTypeLocalItems );
    m_dropTypeMap.insert( 4, SourceTreeItem::DropTypeTop50 );

    m_dropTypeTextMap.insert( 0, "Track" );
    m_dropTypeTextMap.insert( 1, "Album" );
    m_dropTypeTextMap.insert( 2, "Artist" );
    m_dropTypeTextMap.insert( 3, "Local" );
    m_dropTypeTextMap.insert( 4, "Top 10" );

    m_dropTypeImageMap.insert( 0, QPixmap( ":/data/images/drop-song.png" ).scaledToWidth( 32, Qt::SmoothTransformation ) );
    m_dropTypeImageMap.insert( 1, QPixmap( ":/data/images/drop-album.png" ).scaledToWidth( 32, Qt::SmoothTransformation ) );
    m_dropTypeImageMap.insert( 2, QPixmap( ":/data/images/drop-all-songs.png" ).scaledToHeight( 32, Qt::SmoothTransformation ) );
    m_dropTypeImageMap.insert( 3, QPixmap( ":/data/images/drop-local-songs.png" ).scaledToWidth( 32, Qt::SmoothTransformation ) );
    m_dropTypeImageMap.insert( 4, QPixmap( ":/data/images/drop-top-songs.png" ).scaledToWidth( 32, Qt::SmoothTransformation ) );

    m_dropMimeData = new QMimeData();
}

SourceDelegate::~SourceDelegate()
{
    delete m_dropMimeData;
}

QSize
SourceDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    SourceTreeItem *item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();

    if ( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::Collection )
        return QSize( option.rect.width(), 44 );
    else if ( m_expandedMap.contains( index ) )
    {
        if ( !m_expandedMap.value( index )->initialized() )
        {
            int dropTypes = dropTypeCount( item );
            qDebug() << "droptypecount is " << dropTypes;
            QSize originalSize = QStyledItemDelegate::sizeHint( option, index );
            QSize targetSize = originalSize + QSize( 0, dropTypes == 0 ? 0 : 56 );
            m_expandedMap.value( index )->initialize( originalSize, targetSize, 300 );
            m_expandedMap.value( index )->expand();
        }
        QMetaObject::invokeMethod( m_parent, "update", Qt::QueuedConnection, Q_ARG( QModelIndex, index ) );
        return m_expandedMap.value( index )->size();
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
    else if ( (type == SourcesModel::StaticPlaylist || type == SourcesModel::CategoryAdd )
              && m_expandedMap.contains( index ) && m_expandedMap.value( index )->partlyExpanded() && dropTypeCount( item ) > 0 )
    {
        // Let Qt paint the original item. We add our stuff after it
        o.state &= ~QStyle::State_Selected;
        o.showDecorationSelected = false;
        o.rect.adjust( 0, 0, 0, - option.rect.height() + m_expandedMap.value( index )->originalSize().height() );
        QStyledItemDelegate::paint( painter, o, index );

        painter->save();

        // Get whole rect for the menu
        QRect itemsRect = option.rect.adjusted( -option.rect.x(), m_expandedMap.value( index )->originalSize().height(), 0, 0 );

        QPoint cursorPos = m_parent->mapFromGlobal( QCursor::pos() );
        bool cursorInRect = false;
        if ( itemsRect.contains( cursorPos ) )
            cursorInRect = true;

        // draw the background

        QLinearGradient linearGradient( itemsRect.topLeft(), itemsRect.bottomLeft() );
        linearGradient.setColorAt( 0.0, Qt::white );
//        linearGradient.setColorAt( 0.8, QColor( 0xd6, 0xd6, 0xd6 ) ); // light grey
//        linearGradient.setColorAt( 1.0, QColor( 0xf4, 0x17, 0x05 ) ); // dark red
//        linearGradient.setColorAt( 1.0, QColor( 0xb1, 0xb1, 0xb1 ) ); // not so light but still not dark grey
        linearGradient.setColorAt( 0.9, QColor( 0x88, 0x88, 0x88 ) );
        linearGradient.setColorAt( 1.0, QColor( 0x99, 0x99, 0x99 ) ); // dark grey

        QPen pen = painter->pen();
        painter->setPen( QPen( Qt::NoPen ) );
        painter->setBrush( linearGradient );
        painter->drawRect( itemsRect );

        // calculate sizes for the icons
        int totalCount = dropTypeCount( item );
        int itemWidth = itemsRect.width() / totalCount;
        int iconSpacing = ( itemWidth - 32 ) / 2;

        // adjust to one single entry
        itemsRect.adjust( 0, 0, -itemsRect.width() + itemWidth, 0 );

        int count = 0;

        pen.setColor( Qt::white );
        painter->setPen( pen );

        QFont font = painter->font();
        font.setPixelSize( 12 );
        painter->setFont( font );
        QFont fontBold = painter->font();
        fontBold.setBold( true );

        QRect textRect;
        QRect imageRect;

        SourceTreeItem::DropTypes dropTypes = item->supportedDropTypes( m_dropMimeData );

        for ( int i = 0; i < 5; ++i )
        {
            if ( !dropTypes.testFlag( m_dropTypeMap.value( i ) ) )
                continue;


            if ( count > 0 )
                itemsRect.adjust( itemWidth, 0, itemWidth, 0 );

            if ( itemsRect.contains( cursorPos ) | !cursorInRect )
            {
                painter->setFont( fontBold );
                m_hoveredDropType = m_dropTypeMap.value( i );
                cursorInRect = true;
            }
            else
                painter->setFont( font );

            int textSpacing = ( itemWidth - painter->fontMetrics().width( m_dropTypeTextMap.value( i ) ) ) / 2;
            textRect = itemsRect.adjusted( textSpacing - 1, itemsRect.height() - painter->fontMetrics().height() - 2, 0, 0 );
            painter->drawText( textRect, m_dropTypeTextMap.value( i ) );

            int maxHeight = itemsRect.height() - textRect.height() - 2;
            int verticalOffset = qMax( 0, maxHeight - 32 );
            if ( itemsRect.bottom() - textRect.height() - 2 > itemsRect.top() )
            {
                imageRect = itemsRect.adjusted( iconSpacing, verticalOffset, -iconSpacing, -textRect.height() - 2 );
                painter->drawPixmap( imageRect.x(), imageRect.y(), m_dropTypeImageMap.value( i ).copy( 0, 32 - imageRect.height(), 32, imageRect.height() ) );
            }

            count++;
        }


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

bool
SourceDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{

    if ( event->type() == QEvent::MouseButtonRelease )
    {
        SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
        if ( type == SourcesModel::TemporaryPage )
        {
            TemporaryPageItem* gpi = qobject_cast< TemporaryPageItem* >( index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
            Q_ASSERT( gpi );
            QMouseEvent* ev = static_cast< QMouseEvent* >( event );

            QStyleOptionViewItemV4 o = option;
            initStyleOption( &o, index );
            int padding = 3;
            QRect r ( o.rect.right() - padding - m_iconHeight, padding + o.rect.y(), m_iconHeight, m_iconHeight );

            if ( r.contains( ev->pos() ) )
                gpi->removeFromList();
        }
    }

    return QStyledItemDelegate::editorEvent ( event, model, option, index );
}

int
SourceDelegate::dropTypeCount( SourceTreeItem* item ) const
{
    int menuCount = 0;
    if ( item->supportedDropTypes( m_dropMimeData ).testFlag( SourceTreeItem::DropTypeThisTrack ) )
        menuCount++;

    if ( item->supportedDropTypes( m_dropMimeData ).testFlag( SourceTreeItem::DropTypeThisAlbum ) )
        menuCount++;

    if ( item->supportedDropTypes( m_dropMimeData ).testFlag( SourceTreeItem::DropTypeAllFromArtist ) )
        menuCount++;

    if ( item->supportedDropTypes( m_dropMimeData ).testFlag( SourceTreeItem::DropTypeLocalItems ) )
        menuCount++;

    if ( item->supportedDropTypes( m_dropMimeData ).testFlag( SourceTreeItem::DropTypeTop50 ) )
        menuCount++;

    return menuCount;
}

SourceTreeItem::DropType
SourceDelegate::hoveredDropType() const
{
    return m_hoveredDropType;
}

void
SourceDelegate::hovered(const QModelIndex &index, const QMimeData *mimeData)
{
    if ( !index.isValid() )
    {
        foreach ( AnimationHelper *helper, m_expandedMap )
        {
            helper->collapse();
        }
        return;
    }
    if ( !m_expandedMap.contains( index ) )
    {
        foreach ( AnimationHelper *helper, m_expandedMap )
        {
            helper->collapse();
        }

        m_newDropHoverIndex = index;
        m_dropMimeData->clear();
        foreach ( const QString &mimeDataFormat, mimeData->formats() )
        {
            m_dropMimeData->setData( mimeDataFormat, mimeData->data( mimeDataFormat ) );
        }

        m_expandedMap.insert( m_newDropHoverIndex, new AnimationHelper( m_newDropHoverIndex ) );
        connect( m_expandedMap.value( m_newDropHoverIndex ), SIGNAL( finished( QModelIndex ) ), SLOT( animationFinished( QModelIndex ) ) );

    }
    else
        qDebug() << "expandedMap already contains index" << index;
}

void
SourceDelegate::dragLeaveEvent()
{
    foreach ( AnimationHelper *helper, m_expandedMap )
    {
        helper->collapse( true );
    }
}

void
SourceDelegate::animationFinished( const QModelIndex& index )
{
    delete m_expandedMap.take( index );
}
