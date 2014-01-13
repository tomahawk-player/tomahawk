/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013-2014, Teo Mrnjavac <teo@kde.org>
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

#include "PlaylistItemDelegate.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <boost/concept_check.hpp>

#include "Query.h"
#include "Result.h"
#include "Artist.h"
#include "Album.h"
#include "Source.h"
#include "SourceList.h"

#include "PlayableModel.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "TrackView.h"
#include "ViewHeader.h"
#include "ViewManager.h"

#include "utils/PixmapDelegateFader.h"
#include "utils/Closure.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlaylistItemDelegate::PlaylistItemDelegate( TrackView* parent, PlayableProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_smallBoldFontMetrics( QFontMetrics( parent->font() ) )
    , m_bigBoldFontMetrics( QFontMetrics( parent->font() ) )
    , m_view( parent )
    , m_model( proxy )
{
    m_topOption = QTextOption( Qt::AlignTop );
    m_topOption.setWrapMode( QTextOption::NoWrap );
    m_bottomOption = QTextOption( Qt::AlignBottom );
    m_bottomOption.setWrapMode( QTextOption::NoWrap );

    m_centerOption = QTextOption( Qt::AlignVCenter );
    m_centerOption.setWrapMode( QTextOption::NoWrap );
    m_centerRightOption = QTextOption( Qt::AlignVCenter | Qt::AlignRight );
    m_centerRightOption.setWrapMode( QTextOption::NoWrap );

    m_bigBoldFont = parent->font();
    m_bigBoldFont.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
    m_bigBoldFont.setWeight( 99 );

    m_boldFont = parent->font();
    m_boldFont.setBold( true );

    m_smallBoldFont = parent->font();
    m_smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    m_smallBoldFont.setBold( true );
    m_smallBoldFont.setWeight( 60 );

    m_smallFont = parent->font();
    m_smallFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );

    m_bigBoldFontMetrics = QFontMetrics( m_bigBoldFont );
    m_smallBoldFontMetrics = QFontMetrics( m_smallBoldFont );

    connect( this, SIGNAL( updateIndex( QModelIndex ) ), parent, SLOT( update( QModelIndex ) ) );
    connect( proxy, SIGNAL( modelReset() ), SLOT( modelChanged() ) );
    connect( parent, SIGNAL( modelChanged() ), SLOT( modelChanged() ) );
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

    {
        if ( m_model->style() == PlayableProxyModel::Short )
        {
            int rowHeight = option.fontMetrics.height() + 8;
            size.setHeight( rowHeight * 2 );
        }
        else if ( m_model->style() == PlayableProxyModel::Detailed )
        {
            int rowHeight = option.fontMetrics.height() * 1.6;
            size.setHeight( rowHeight );
        }
    }

    return size;
}


void
PlaylistItemDelegate::prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item ) const
{
    initStyleOption( option, index );

    TomahawkUtils::prepareStyleOption( option, index, item );
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    const int style = index.data( PlayableProxyModel::StyleRole ).toInt();
    switch ( style )
    {
        case PlayableProxyModel::Detailed:
            paintDetailed( painter, option, index );
            break;

        case PlayableProxyModel::Short:
            paintShort( painter, option, index );
            break;
    }
}


void
PlaylistItemDelegate::paintShort( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );
    opt.text.clear();

    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( m_view->header()->visualIndex( index.column() ) > 0 )
        return;

    const track_ptr track = item->query()->track();
    QPixmap pixmap;
    QString upperLeftText, upperRightText, lowerText;

    if ( !item->playbackLog().source )
    {
        upperLeftText = track->track();
        lowerText = track->artist();
    }
    else
    {
        upperLeftText = track->track();
        upperRightText = QString( " - %2" ).arg( track->artist() );
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->playbackLog().timestamp ), true );

        if ( item->playbackLog().source->isLocal() )
            lowerText = QString( tr( "played %1 by you" ) ).arg( playtime );
        else
            lowerText = QString( tr( "played %1 by %2" ) ).arg( playtime ).arg( item->playbackLog().source->friendlyName() );
    }

    painter->save();
    {
        QRect r = opt.rect.adjusted( 3, 6, 0, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() )
        {
            const int pixMargin = 2;
            const int pixHeight = r.height() - pixMargin * 2;
            const QRect npr = r.adjusted( pixMargin, pixMargin + 1, pixHeight - r.width() + pixMargin, -pixMargin + 1 );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight + 8, 0, 0, 0 );
        }

        painter->setPen( opt.palette.text().color() );

        QRect ir = r.adjusted( 4, 0, -option.rect.width() + option.rect.height() - 8 + r.left(), 0 );
        pixmap = item->query()->track()->cover( ir.size(), true );

        if ( pixmap.isNull() )
        {
            pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::RoundedCorners, ir.size() );
        }

        painter->drawPixmap( ir, pixmap );

        r.adjust( ir.width() + 12, 0, -12, 0 );
        painter->setFont( m_boldFont );
        QFontMetrics fm = painter->fontMetrics();
        QString elided = fm.elidedText( upperLeftText, Qt::ElideRight, r.width() );
        if ( fm.width( elided ) != fm.width( upperLeftText ) ) //if we had to elide the track title
        {                                                      //we just paint that and we're done
            painter->drawText( r.adjusted( 0, 1, 0, 0 ), elided, m_topOption );
        }
        else
        {
            int remainingSpace = r.width() - fm.width( upperLeftText );
            elided = fm.elidedText( upperRightText, Qt::ElideRight, remainingSpace );
            painter->drawText( r.adjusted( 0, 1, -remainingSpace, 0 ), upperLeftText, m_topOption );

            if ( item->query()->numResults() > 0 && item->query()->results().first()->isOnline() )
                painter->setPen( opt.palette.text().color().lighter( 220 ) );

            painter->drawText( r.adjusted( r.width() - remainingSpace, 1, 0, 0 ), elided, m_topOption );
        }

        painter->setFont( opt.font );
        if ( !( option.state & QStyle::State_Selected || item->isPlaying() ) )
            painter->setPen( Qt::gray );

        elided = painter->fontMetrics().elidedText( lowerText, Qt::ElideRight, r.width() );
        painter->drawText( r.adjusted( 0, 1, 0, 0 ), elided, m_bottomOption );
    }

    painter->restore();
}


void
PlaylistItemDelegate::paintDetailed( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QTextOption textOption( Qt::AlignVCenter | (Qt::Alignment)index.data( Qt::TextAlignmentRole ).toUInt() );
    textOption.setWrapMode( QTextOption::NoWrap );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );
    opt.text.clear();
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( m_hoveringOver == index && !index.data().toString().isEmpty() &&
       ( index.column() == PlayableModel::Artist || index.column() == PlayableModel::Album || index.column() == PlayableModel::Track ) )
    {
        opt.rect.setWidth( opt.rect.width() - opt.rect.height() - 2 );
        const QRect arrowRect( opt.rect.x() + opt.rect.width(), opt.rect.y() + 1, opt.rect.height() - 2, opt.rect.height() - 2 );
        drawInfoButton( painter, arrowRect, index, 0.9 );
    }

    painter->save();

/*    if ( index.column() == PlayableModel::Score )
    {
        QColor barColor( 167, 183, 211 ); // This matches the sidebar (sourcetreeview.cpp:672)
        if ( opt.state & QStyle::State_Selected && !item->isPlaying() )
            painter->setPen( Qt::white );
        else
            painter->setPen( barColor );

        QRect r = opt.rect.adjusted( 3, 3, -6, -4 );
        painter->drawRect( r );

        QRect fillR = r;
        int fillerWidth = (int)( index.data().toFloat() * (float)fillR.width() );
        fillR.adjust( 0, 0, -( fillR.width() - fillerWidth ), 0 );

        if ( opt.state & QStyle::State_Selected && !item->isPlaying() )
            painter->setBrush( TomahawkUtils::Colors::NOW_PLAYING_ITEM.lighter() );
        else
            painter->setBrush( barColor );

        painter->drawRect( fillR );
    }
    else */
    if ( item->isPlaying() )
    {
        QRect r = opt.rect.adjusted( 3, 0, 0, 0 );

        // Paint Now Playing Speaker Icon
        if ( m_view->header()->visualIndex( index.column() ) == 0 )
        {
            const int pixMargin = 1;
            const int pixHeight = r.height() - pixMargin * 2;
            const QRect npr = r.adjusted( pixMargin, pixMargin, pixHeight - r.width() + pixMargin, -pixMargin );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight + 6, 0, 0, 0 );
        }

        painter->setPen( opt.palette.text().color() );
        const QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, r.width() - 3 );
        painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, textOption );
    }
    else
    {
        painter->setPen( opt.palette.text().color() );
        const QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, opt.rect.width() - 6 );
        painter->drawText( opt.rect.adjusted( 3, 1, -3, 0 ), text, textOption );
    }

    painter->restore();
}


QRect
PlaylistItemDelegate::drawInfoButton( QPainter* painter, const QRect& rect, const QModelIndex& index, float height ) const
{
    const int iconSize = rect.height() * height;
    QRect pixmapRect = QRect( ( rect.height() - iconSize ) / 2 + rect.left(), rect.center().y() - iconSize / 2, iconSize, iconSize );

    painter->drawPixmap( pixmapRect, TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, pixmapRect.size() ) );
    m_infoButtonRects[ index ] = pixmapRect;

    return rect.adjusted( rect.height(), 0, 0, 0 );
}


QRect
PlaylistItemDelegate::drawCover( QPainter* painter, const QRect& rect, PlayableItem* item, const QModelIndex& index ) const
{
    QRect pixmapRect = rect;
    pixmapRect.setWidth( pixmapRect.height() );

    if ( !m_pixmaps.contains( index ) )
    {
        m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->query(), pixmapRect.size(), TomahawkUtils::RoundedCorners, false ) ) );
        _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<PlaylistItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
        closure->setAutoDelete( false );
    }

    const QPixmap pixmap = m_pixmaps[ index ]->currentPixmap();
    painter->drawPixmap( pixmapRect, pixmap );

    return rect.adjusted( pixmapRect.width(), 0, 0, 0 );
}


QRect
PlaylistItemDelegate::drawLoveBox( QPainter* painter, const QRect& rect, PlayableItem* item, const QModelIndex& index ) const
{
    const int avatarSize = rect.height() - 4 * 2;
    const int avatarMargin = 2;

    QList< Tomahawk::source_ptr > sources;
    foreach ( const Tomahawk::SocialAction& sa, item->query()->queryTrack()->socialActions( "Love", true, true ) )
    {
        sources << sa.source;
    }
    const int max = 5;
    const unsigned int count = qMin( sources.count(), max );

    QRect innerRect = rect.adjusted( rect.width() -
                                     ( avatarSize + avatarMargin ) * ( count + 1 ) -
                                     4 * 4,
                                     0, 0, 0 );

    if ( !sources.isEmpty() )
        drawRectForBox( painter, innerRect );

    QRect avatarsRect = innerRect.adjusted( 4, 4, -4, -4 );

    drawAvatarsForBox( painter, avatarsRect, avatarSize, avatarMargin, count, sources, index );

    TomahawkUtils::ImageType type = item->query()->queryTrack()->loved() ? TomahawkUtils::Loved : TomahawkUtils::NotLoved;
    QRect r = innerRect.adjusted( innerRect.width() - rect.height() + 4, 4, -4, -4 );
    painter->drawPixmap( r, TomahawkUtils::defaultPixmap( type, TomahawkUtils::Original, QSize( r.height(), r.height() ) ) );
    m_loveButtonRects[ index ] = r;

    return rect;
}


QRect
PlaylistItemDelegate::drawGenericBox( QPainter* painter,
                                      const QStyleOptionViewItem& option,
                                      const QRect& rect, const QString& text,
                                      const QList< Tomahawk::source_ptr >& sources,
                                      const QModelIndex& index ) const
{
    const int avatarSize = rect.height() - 4 * 2;
    const int avatarMargin = 2;

    const int max = 5;
    const unsigned int count = qMin( sources.count(), max );

    QTextDocument textDoc;
    textDoc.setHtml( QString( "<b>%1</b>" ).arg( text ) );
    textDoc.setDocumentMargin( 0 );
    textDoc.setDefaultFont( painter->font() );
    textDoc.setDefaultTextOption( m_bottomOption );

    QRect innerRect = rect.adjusted( rect.width() - ( avatarSize + avatarMargin ) * count - 4 * 4 -
                                     textDoc.idealWidth(),
                                     0, 0, 0 );

    QRect textRect = innerRect.adjusted( 4, 4, - innerRect.width() + textDoc.idealWidth() + 2*4, -4 );

    drawRichText( painter, option, textRect, Qt::AlignVCenter|Qt::AlignRight, textDoc );

    if ( !sources.isEmpty() )
        drawRectForBox( painter, innerRect );

    QRect avatarsRect = innerRect.adjusted( textDoc.idealWidth() + 3*4, 4, -4, -4 );
    drawAvatarsForBox( painter, avatarsRect, avatarSize, avatarMargin, count, sources, index );

    return rect;
}


void
PlaylistItemDelegate::drawRectForBox( QPainter* painter, const QRect& rect ) const
{
    painter->save();

    painter->setRenderHint( QPainter::Antialiasing, true );
    painter->setBrush( Qt::transparent );
    QPen pen = painter->pen().color();
    pen.setWidthF( 0.2 );
    painter->setPen( pen );

    painter->drawRoundedRect( rect, 4, 4, Qt::RelativeSize );

    painter->restore();
}


void
PlaylistItemDelegate::drawAvatarsForBox( QPainter* painter,
                                         const QRect& avatarsRect,
                                         int avatarSize,
                                         int avatarMargin,
                                         int count,
                                         const QList< Tomahawk::source_ptr >& sources,
                                         const QModelIndex& index ) const
{
    painter->save();

    QHash< Tomahawk::source_ptr, QRect > rectsToSave;

    unsigned int i = 0;
    foreach ( const Tomahawk::source_ptr& s, sources )
    {
        if ( i >= count )
            break;

        QRect r = avatarsRect.adjusted( ( avatarSize + avatarMargin ) * i, 0, 0, 0 );
        r.setWidth( avatarSize + avatarMargin );

        QPixmap pixmap = s->avatar( TomahawkUtils::Original, QSize( avatarSize, avatarSize ) );

        if ( pixmap.isNull() )
            pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultSourceAvatar, TomahawkUtils::Original, QSize( r.height(), r.height() ) );
        painter->drawPixmap( r.adjusted( avatarMargin/2, 0, -(avatarMargin/2), 0 ), pixmap );

        rectsToSave.insert( s, r );

        i++;
    }

    if ( !rectsToSave.isEmpty() )
        m_avatarBoxRects.insert( index, rectsToSave );

    painter->restore();
}

void
PlaylistItemDelegate::drawRichText( QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, int flags, QTextDocument& text ) const
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


QRect
PlaylistItemDelegate::drawSourceIcon( QPainter* painter, const QRect& rect, PlayableItem* item, float height ) const
{
    const int sourceIconSize = rect.height() * height;
    QRect resultRect = rect.adjusted( 0, 0, -( sourceIconSize + 8 ), 0 );
    if ( item->query()->numResults() == 0 || !item->query()->results().first()->isOnline() )
        return resultRect;

    const QPixmap sourceIcon = item->query()->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) );
    if ( sourceIcon.isNull() )
        return resultRect;

    painter->setOpacity( 0.8 );
    painter->drawPixmap( QRect( rect.right() - sourceIconSize, rect.center().y() - sourceIconSize / 2, sourceIcon.width(), sourceIcon.height() ), sourceIcon );
    painter->setOpacity( 1.0 );

    return resultRect;
}


bool
PlaylistItemDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    QStyledItemDelegate::editorEvent( event, model, option, index );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove &&
         event->type() != QEvent::Leave )
    {
        return false;
    }

    bool hoveringInfo = false;
    bool hoveringLove = false;
    Tomahawk::source_ptr hoveredAvatar;
    QRect hoveredAvatarRect;
    if ( m_infoButtonRects.contains( index ) )
    {
        const QRect infoRect = m_infoButtonRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringInfo = infoRect.contains( ev->pos() );
    }
    if ( m_loveButtonRects.contains( index ) )
    {
        const QRect loveRect = m_loveButtonRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringLove = loveRect.contains( ev->pos() );
    }
    if ( m_avatarBoxRects.contains( index ) )
    {
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        for ( QHash< Tomahawk::source_ptr, QRect >::const_iterator it = m_avatarBoxRects[ index ].constBegin();
              it != m_avatarBoxRects[ index ].constEnd(); ++it )
        {
            if ( it.value().contains( ev->pos() ) )
            {
                hoveredAvatar = it.key();
                hoveredAvatarRect = it.value();
                break;
            }
        }
    }

    if ( event->type() == QEvent::MouseMove )
    {
        if ( hoveringInfo || hoveringLove )
            m_view->setCursor( Qt::PointingHandCursor );
        else
            m_view->setCursor( Qt::ArrowCursor );

        if ( !hoveredAvatar.isNull() )
        {
            const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
            QToolTip::showText( m_view->mapToGlobal( hoveredAvatarRect.bottomLeft() ),
                                hoveredAvatar->friendlyName(),
                                m_view,
                                hoveredAvatarRect );
        }

        if ( m_hoveringOver != index )
        {
            QPersistentModelIndex ti = m_hoveringOver;
            m_hoveringOver = index;

            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( ti ) );
            item->requestRepaint();
            emit updateIndex( m_hoveringOver );
        }

        // We return false here so the view can still decide to process/trigger things like D&D events
        return false;
    }

    // reset mouse cursor. we switch to a pointing hand cursor when hovering a button
    m_view->setCursor( Qt::ArrowCursor );

    if ( event->type() == QEvent::MouseButtonRelease )
    {
        PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
        if ( !item )
            return false;

        if ( hoveringLove )
        {
            item->query()->queryTrack()->setLoved( !item->query()->queryTrack()->loved() );
        }
        else if ( hoveringInfo )
        {
            if ( m_model->style() != PlayableProxyModel::Detailed )
            {
                if ( item->query() )
                    ViewManager::instance()->show( item->query()->track()->toQuery() );
            }
            else
            {
                switch ( index.column() )
                {
                    case PlayableModel::Artist:
                    {
                        ViewManager::instance()->show( item->query()->track()->artistPtr() );
                        break;
                    }

                    case PlayableModel::Album:
                    {
                        ViewManager::instance()->show( item->query()->track()->albumPtr() );
                        break;
                    }

                    case PlayableModel::Track:
                    {
                        ViewManager::instance()->show( item->query()->track()->toQuery() );
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        event->accept();
        return true;
    }

    return false;
}


void
PlaylistItemDelegate::resetHoverIndex()
{
    if ( !m_model )
        return;

    QPersistentModelIndex idx = m_hoveringOver;

    m_hoveringOver = QModelIndex();
    m_infoButtonRects.clear();
    m_loveButtonRects.clear();

    QModelIndex itemIdx = m_model->mapToSource( idx );
    if ( itemIdx.isValid() )
    {
        PlayableItem* item = m_model->sourceModel()->itemFromIndex( itemIdx );
        if ( item )
            item->requestRepaint();
    }

    emit updateIndex( idx );
    m_view->repaint();
}


void
PlaylistItemDelegate::modelChanged()
{
    m_pixmaps.clear();
}


void
PlaylistItemDelegate::doUpdateIndex( const QPersistentModelIndex& index )
{
    if ( index.isValid() )
        emit updateIndex( index );
}
