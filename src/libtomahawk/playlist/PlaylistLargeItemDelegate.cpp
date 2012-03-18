/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PlaylistLargeItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

#include "query.h"
#include "result.h"
#include "artist.h"
#include "source.h"
#include "sourcelist.h"

#include "trackmodel.h"
#include "trackmodelitem.h"
#include "trackproxymodel.h"
#include "trackview.h"
#include "trackheader.h"

#include "utils/tomahawkutilsgui.h"
#include "utils/logger.h"

using namespace Tomahawk;


PlaylistLargeItemDelegate::PlaylistLargeItemDelegate( TrackView* parent, TrackProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    m_topOption = QTextOption( Qt::AlignTop );
    m_topOption.setWrapMode( QTextOption::NoWrap );

    m_centerRightOption = QTextOption( Qt::AlignVCenter | Qt::AlignRight );
    m_centerRightOption.setWrapMode( QTextOption::NoWrap );

    m_bottomOption = QTextOption( Qt::AlignBottom );
    m_bottomOption.setWrapMode( QTextOption::NoWrap );
}


QSize
PlaylistLargeItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    if ( index.isValid() )
    {
        int rowHeight = option.fontMetrics.height() + 8;
        size.setHeight( rowHeight * 3 );
    }

    return size;
}


QWidget*
PlaylistLargeItemDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED( parent );
    Q_UNUSED( option );
    Q_UNUSED( index );
    return 0;
}


void
PlaylistLargeItemDelegate::prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, TrackModelItem* item ) const
{
    initStyleOption( option, index );

    if ( item->isPlaying() )
    {
        option->palette.setColor( QPalette::Highlight, option->palette.color( QPalette::Mid ) );
        option->state |= QStyle::State_Selected;
    }

    if ( option->state & QStyle::State_Selected )
    {
        option->palette.setColor( QPalette::Text, option->palette.color( QPalette::HighlightedText ) );
    }
    else
    {
        float opacity = 0.0;
        if ( item->query()->results().count() )
            opacity = item->query()->results().first()->score();

        opacity = qMax( (float)0.3, opacity );
        QColor textColor = TomahawkUtils::alphaBlend( option->palette.color( QPalette::Text ), option->palette.color( QPalette::BrightText ), opacity );

        option->palette.setColor( QPalette::Text, textColor );
    }
}


void
PlaylistLargeItemDelegate::drawRichText( QPainter* painter, const QRect& rect, int flags, QTextDocument& text ) const
{
    text.setPageSize( QSize( rect.width(), QWIDGETSIZE_MAX ) );
    QAbstractTextDocumentLayout* layout = text.documentLayout();

    const int height = qRound( layout->documentSize().height() );
    int y = rect.y();
    if ( flags & Qt::AlignBottom )
        y += ( rect.height() - height );
    else if ( flags & Qt::AlignVCenter )
        y += ( rect.height() - height ) / 2;

    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor( QPalette::Text, painter->pen().color() );

    painter->save();
    painter->translate( rect.x(), y );
    layout->draw( painter, context );
    painter->restore();
}


void
PlaylistLargeItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    TrackModelItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );
    opt.text.clear();

    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( m_view->header()->visualIndex( index.column() ) > 0 )
        return;

    QPixmap pixmap, avatar;
    QString artist, track, upperText, lowerText;
    source_ptr source = item->query()->playedBy().first;
    unsigned int duration = 0;

    if ( item->query()->results().count() )
    {
        artist = item->query()->results().first()->artist()->name();
        track = item->query()->results().first()->track();
        duration = item->query()->results().first()->duration();
    }
    else
    {
        artist = item->query()->artist();
        track = item->query()->track();
    }

    lowerText = item->query()->socialActionDescription( "Love", Query::Detailed );

    if ( source.isNull() )
    {
    }
    else
    {
        upperText = QString( "%1 - %2" ).arg( artist ).arg( track );
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->query()->playedBy().second ), true );

        if ( source == SourceList::instance()->getLocal() )
            lowerText = QString( tr( "played %1 by you" ) ).arg( playtime );
        else
            lowerText = QString( tr( "played %1 by %2" ) ).arg( playtime ).arg( source->friendlyName() );
    }

    painter->save();
    {
        QRect r = opt.rect.adjusted( 3, 6, 0, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() )
        {
            QPixmap nowPlayingIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker );
            QRect npr = r.adjusted( 3, r.height() / 2 - nowPlayingIcon.height() / 2, 18 - r.width(), -r.height() / 2 + nowPlayingIcon.height() / 2 );
            nowPlayingIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() );
            painter->drawPixmap( npr, nowPlayingIcon );
            r.adjust( 22, 0, 0, 0 );
        }

        painter->setPen( opt.palette.text().color() );

        QSize avatarSize( 32, 32 );
        QRect pixmapRect = r.adjusted( 6, 0, -option.rect.width() + option.rect.height() - 6 + r.left(), 0 );
        QRect avatarRect = r.adjusted( option.rect.width() - r.left() - 12 - avatarSize.width(), ( option.rect.height() - avatarSize.height() ) / 2 - 5, 0, 0 );
        avatarRect.setSize( avatarSize );
        
        if ( source )
            avatar = source->avatar( Source::FancyStyle, avatarRect.size() );

        pixmap = item->query()->cover( pixmapRect.size(), false );
        if ( !pixmap )
        {
            pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::ScaledCover, pixmapRect.size() );
        }

        painter->drawPixmap( pixmapRect, pixmap );
        
        if ( !avatar.isNull() )
            painter->drawPixmap( avatarRect, avatar );

        QFont boldFont = opt.font;
        boldFont.setPixelSize( 15 );
        boldFont.setWeight( 99 );

        QFont smallBoldFont = opt.font;
        smallBoldFont.setPixelSize( 12 );
        smallBoldFont.setBold( true );
        smallBoldFont.setWeight( 60 );

        QFont smallFont = opt.font;
        smallFont.setPixelSize( 10 );

        r.adjust( pixmapRect.width() + 12, 1, -28 - avatar.width(), 0 );
        QRect leftRect = r.adjusted( 0, 0, -48, 0 );
        QRect rightRect = r.adjusted( r.width() - 40, 0, 0, 0 );

        painter->setFont( boldFont );
        QString text = painter->fontMetrics().elidedText( track, Qt::ElideRight, leftRect.width() );
        painter->drawText( leftRect, text, m_topOption );

        painter->setFont( smallBoldFont );
        text = painter->fontMetrics().elidedText( artist, Qt::ElideRight, leftRect.width() );
        painter->drawText( leftRect.adjusted( 0, 19, 0, 0 ), text, m_topOption );

        painter->setFont( smallFont );
        painter->setPen( Qt::gray );
        QTextDocument textDoc;
        textDoc.setHtml( lowerText );
        textDoc.setDocumentMargin( 0 );
        textDoc.setDefaultFont( painter->font() );
        textDoc.setDefaultTextOption( m_bottomOption );
        
        if ( textDoc.idealWidth() > leftRect.width() )
            textDoc.setHtml( item->query()->socialActionDescription( "Love", Query::Short ) );

        drawRichText( painter, leftRect, Qt::AlignBottom, textDoc );

        if ( duration > 0 )
        {
            painter->setFont( smallBoldFont );
            text = painter->fontMetrics().elidedText( TomahawkUtils::timeToString( duration ), Qt::ElideRight, rightRect.width() );
            painter->drawText( rightRect, text, m_centerRightOption );
        }
    }
    painter->restore();
}
