#include "playlistitemdelegate.h"

#include <QDebug>
#include <QPainter>

#include "tomahawk/query.h"
#include "tomahawk/result.h"
#include "playlistview.h"
#include "playlistitem.h"
#include "playlistmodel.h"


PlaylistItemDelegate::PlaylistItemDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
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
    return size; // + QSize( 0, 3 ); // FIXME: a hack for now (due to UniformRowHeights)

    PlaylistItem* item = PlaylistModel::indexToPlaylistItem( index );
    if ( !item )
        return size;

    if ( item->beingRemoved() && m_removalProgress > 0.0 && m_removalProgress < 50.0 )
    {
        int h = (((qreal)m_removalProgress * 2.0 ) / (qreal)100.0 ) * (qreal)size.height();
        if ( h < 2 )
            h = 0;
        return QSize( size.width(), h );
    }
    else
        return size;

    return size;
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlaylistItem* item = PlaylistModel::indexToPlaylistItem( index );
    if ( item )
    {
        if ( item->beingRemoved() )
            painter->setOpacity( (qreal)m_removalProgress / (qreal)100.0 );
        else
        {
            if ( item->query()->results().count() )
                painter->setOpacity( item->query()->results().at( 0 )->score() );
            else
                painter->setOpacity( 0.3 );
        }
    }

    QStyledItemDelegate::paint( painter, option, index );
}
