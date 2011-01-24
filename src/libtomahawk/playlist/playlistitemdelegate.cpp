#include "playlistitemdelegate.h"

#include <QDebug>
#include <QPainter>
#include <QAbstractItemView>

#include "query.h"
#include "result.h"

#include "playlist/plitem.h"
#include "playlist/trackproxymodel.h"
#include "playlist/trackview.h"
#include "playlist/trackheader.h"

#include "utils/tomahawkutils.h"

#define PLAYING_ICON QString( RESPATH "images/now-playing-speaker.png" )


PlaylistItemDelegate::PlaylistItemDelegate( TrackView* parent, TrackProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
}


void
PlaylistItemDelegate::updateRowSize( const QModelIndex& index )
{
    emit sizeHintChanged( index );
}


QSize
PlaylistItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );
    return size;
}


QWidget*
PlaylistItemDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    return 0;
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item || item->query().isNull() )
        return;

    if ( item->query()->results().count() )
        painter->setOpacity( item->query()->results().at( 0 )->score() );
    else
        painter->setOpacity( 0.3 );

    if ( item->isPlaying() )
    {
        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );

        {
            QRect r = option.rect.adjusted( 3, 0, 0, -3 );
            if ( m_view->header()->visualIndex( index.column() ) == 0 )
            {
                painter->drawPixmap( r.adjusted( 3, 3, 18 - r.width(), 0 ), QPixmap( PLAYING_ICON ) );
                r = r.adjusted( 22, 0, 0, 0 );
            }

            painter->setPen( option.palette.text().color() );
            painter->drawText( r.adjusted( 0, 2, 0, 0 ), index.data().toString() );
        }

        if ( m_view->header()->visualIndex( index.column() ) == m_view->header()->visibleSectionCount()  - 1 )
        {
            QRect r = QRect( 3, option.rect.y() + 1, m_view->viewport()->width() - 6, option.rect.height() - 2 );
            painter->setPen( option.palette.highlight().color() );
            QPen pen = painter->pen();
            pen.setWidth( 1.0 );
            painter->setPen( pen );
            painter->drawRoundedRect( r, 3.0, 3.0 );
        }

        painter->restore();
    }
    else
    {
        QStyledItemDelegate::paint( painter, option, index );
    }
}
