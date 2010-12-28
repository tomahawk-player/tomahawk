#include "albumitemdelegate.h"

#include <QDebug>
#include <QPainter>
#include <QAbstractItemView>

#include "query.h"
#include "result.h"
#include "tomahawk/tomahawkapp.h"

#include "playlist/albumitem.h"
#include "playlist/albumproxymodel.h"


AlbumItemDelegate::AlbumItemDelegate( QAbstractItemView* parent, AlbumProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
}


QSize
AlbumItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );
    return size;
}


void
AlbumItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    AlbumItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    APP->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( option.state & QStyle::State_Selected )
    {
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
    }

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

    painter->drawPixmap( option.rect.adjusted( 4, 4, -4, -38 ), QPixmap( RESPATH "images/cover-shadow.png" ) );
    painter->drawPixmap( option.rect.adjusted( 6, 4, -6, -41 ), item->cover );

    QTextOption to;
    to.setAlignment( Qt::AlignHCenter );
    QFont font = opt.font;
    QFont boldFont = opt.font;
    boldFont.setBold( true );

    painter->drawText( option.rect.adjusted( 0, option.rect.height() - 16, 0, -2 ), item->album()->artist()->name(), to );

    painter->setFont( boldFont );
    painter->drawText( option.rect.adjusted( 0, option.rect.height() - 32, 0, -18 ), item->album()->name(), to );

    painter->restore();
}
