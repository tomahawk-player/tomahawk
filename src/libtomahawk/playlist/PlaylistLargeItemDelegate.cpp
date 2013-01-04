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

#include "Query.h"
#include "Result.h"
#include "Artist.h"
#include "Source.h"
#include "SourceList.h"

#include "PlaylistView.h"
#include "PlayableModel.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "TrackView.h"
#include "ViewHeader.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include <utils/PixmapDelegateFader.h>
#include <utils/Closure.h>

using namespace Tomahawk;


PlaylistLargeItemDelegate::PlaylistLargeItemDelegate( DisplayMode mode, TrackView* parent, PlayableProxyModel* proxy )
    : PlaylistItemDelegate( parent, proxy )
    , m_view( parent )
    , m_model( proxy )
    , m_mode( mode )
{
    m_topOption = QTextOption( Qt::AlignTop );
    m_topOption.setWrapMode( QTextOption::NoWrap );

    m_centerRightOption = QTextOption( Qt::AlignVCenter | Qt::AlignRight );
    m_centerRightOption.setWrapMode( QTextOption::NoWrap );

    m_bottomOption = QTextOption( Qt::AlignBottom );
    m_bottomOption.setWrapMode( QTextOption::NoWrap );

    connect( proxy, SIGNAL( modelReset() ), SLOT( modelChanged() ) );
    connect( parent, SIGNAL( modelChanged() ), SLOT( modelChanged() ) );
}


QSize
PlaylistLargeItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    int rowHeight = option.fontMetrics.height() + 8;
    size.setHeight( rowHeight * 3 );

    return size;
}


void
PlaylistLargeItemDelegate::drawRichText( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, int flags, QTextDocument& text ) const
{
    Q_UNUSED( option );

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
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );
    opt.text.clear();

    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( m_view->header()->visualIndex( index.column() ) > 0 )
        return;

    const query_ptr q = item->query()->displayQuery();
    const QString artist = q->artist();
    const QString album = q->album();
    const QString track = q->track();
    int duration = q->duration();
    QString lowerText;

    QSize avatarSize( 32, 32 );
    source_ptr source = item->query()->playedBy().first;
    if ( m_mode == RecentlyPlayed && !source.isNull() )
    {
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->query()->playedBy().second ), true );

        if ( source == SourceList::instance()->getLocal() )
            lowerText = QString( tr( "played %1 by you", "e.g. played 3 hours ago by you" ) ).arg( playtime );
        else
            lowerText = QString( tr( "played %1 by %2", "e.g. played 3 hours ago by SomeSource" ) ).arg( playtime ).arg( source->friendlyName() );
    }

    if ( m_mode == LatestAdditions && item->query()->numResults() )
    {
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->query()->results().first()->modificationTime() ), true );

        lowerText = QString( tr( "added %1", "e.g. added 3 hours ago" ) ).arg( playtime );
    }

    if ( m_mode == LovedTracks )
        lowerText = item->query()->socialActionDescription( "Love", Query::Detailed );

    painter->save();
    {
        QRect r = opt.rect.adjusted( 4, 6, 0, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() )
        {
            const int pixMargin = 4;
            const int pixHeight = r.height() - pixMargin * 2;
            QRect npr = r.adjusted( pixMargin, pixMargin + 1, pixHeight - r.width() + pixMargin, -pixMargin + 1 );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight + 8, 0, 0, 0 );
        }

        painter->setPen( opt.palette.text().color() );

        QRect pixmapRect = r.adjusted( 6, 0, -option.rect.width() + option.rect.height() - 6 + r.left(), 0 );
        QRect avatarRect = r.adjusted( option.rect.width() - r.left() - 12 - avatarSize.width(), ( option.rect.height() - avatarSize.height() ) / 2 - 5, 0, 0 );
        avatarRect.setSize( avatarSize );

        if ( !m_pixmaps.contains( index ) )
        {
            m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->query(), pixmapRect.size(), TomahawkUtils::RoundedCorners, false ) ) );
            _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<PlaylistLargeItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
            closure->setAutoDelete( false );
        }

        const QPixmap pixmap = m_pixmaps[ index ]->currentPixmap();
        painter->drawPixmap( pixmapRect, pixmap );

        QFont boldFont = opt.font;
        boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
        boldFont.setWeight( 99 );
        QFontMetrics boldFontMetrics( boldFont );

        QFont smallBoldFont = opt.font;
        smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
        smallBoldFont.setBold( true );
        smallBoldFont.setWeight( 60 );
        QFontMetrics smallBoldFontMetrics( smallBoldFont );

        QFont smallFont = opt.font;
        smallFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );

        r.adjust( pixmapRect.width() + 12, 1, - 16, 0 );
        QRect leftRect = r.adjusted( 0, 0, -48, 0 );
        QRect rightRect = r.adjusted( r.width() - smallBoldFontMetrics.width( TomahawkUtils::timeToString( duration ) ), 0, 0, 0 );

        painter->setFont( boldFont );
        QString text = painter->fontMetrics().elidedText( track, Qt::ElideRight, leftRect.width() );
        painter->drawText( leftRect, text, m_topOption );

        painter->setFont( smallFont );
        QTextDocument textDoc;
        if ( album.isEmpty() )
            textDoc.setHtml( tr( "by <b>%1</b>", "e.g. by SomeArtist" ).arg( artist ) );
        else
            textDoc.setHtml( tr( "by <b>%1</b> on <b>%2</b>", "e.g. by SomeArtist on SomeAlbum" ).arg( artist ).arg( album ) );
        textDoc.setDocumentMargin( 0 );
        textDoc.setDefaultFont( painter->font() );
        textDoc.setDefaultTextOption( m_topOption );

        drawRichText( painter, opt, leftRect.adjusted( 0, boldFontMetrics.height() + 1, 0, 0 ), Qt::AlignTop, textDoc );

        if ( !( option.state & QStyle::State_Selected || item->isPlaying() ) )
            painter->setPen( Qt::gray );

        textDoc.setHtml( lowerText );
        textDoc.setDocumentMargin( 0 );
        textDoc.setDefaultFont( painter->font() );
        textDoc.setDefaultTextOption( m_bottomOption );

        if ( textDoc.idealWidth() > leftRect.width() )
            textDoc.setHtml( item->query()->socialActionDescription( "Love", Query::Short ) );

        drawRichText( painter, opt, leftRect, Qt::AlignBottom, textDoc );

        const int sourceIconSize = avatarRect.width() - 6;
        if ( q->numResults() && !q->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) ).isNull() )
        {
            const QPixmap sourceIcon = q->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) );
            painter->setOpacity( 0.8 );
            painter->drawPixmap( QRect( rightRect.right() - sourceIconSize, r.center().y() - sourceIconSize / 2, sourceIcon.width(), sourceIcon.height() ), sourceIcon );
            painter->setOpacity( 1.0 );
            rightRect.moveLeft( rightRect.left() - sourceIcon.width() - 8 );
        }

        if ( duration > 0 )
        {
            painter->setPen( opt.palette.text().color() );
            painter->setFont( smallBoldFont );
            painter->drawText( rightRect, TomahawkUtils::timeToString( duration ), m_centerRightOption );
        }
    }
    painter->restore();
}


void
PlaylistLargeItemDelegate::doUpdateIndex( const QPersistentModelIndex& idx )
{
    if ( idx.isValid() )
        emit updateIndex( idx );
}


void
PlaylistLargeItemDelegate::modelChanged()
{
    m_pixmaps.clear();
}
