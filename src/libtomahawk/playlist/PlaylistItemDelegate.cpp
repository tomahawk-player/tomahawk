/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "widgets/DownloadButton.h"
#include "audio/AudioEngine.h"
#include "utils/ImageRegistry.h"
#include "utils/PixmapDelegateFader.h"
#include "utils/Closure.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

using namespace Tomahawk;


PlaylistItemDelegate::PlaylistItemDelegate( TrackView* parent, PlayableProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
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

    m_demiBoldFont = parent->font();
    m_demiBoldFont.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    m_demiBoldFont.setWeight( QFont::DemiBold );

    m_normalFont = parent->font();
    m_normalFont.setPointSize( TomahawkUtils::defaultFontSize() + 1 );

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
        if ( m_model->style() != PlayableProxyModel::SingleColumn )
        {
            int rowHeight = option.fontMetrics.height() * 1.6;
            size.setHeight( rowHeight );
        }
    }

    return size;
}


void
PlaylistItemDelegate::prepareStyleOption( QStyleOptionViewItem* option, const QModelIndex& index, PlayableItem* item ) const
{
    initStyleOption( option, index );

    TomahawkUtils::prepareStyleOption( option, index, item );
}


QWidget*
PlaylistItemDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    return DownloadButton::handleCreateEditor( parent, item->query(), m_view, index );
}


void
PlaylistItemDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyledItemDelegate::updateEditorGeometry( editor, option, index );

    DownloadButton* comboBox = static_cast<DownloadButton*>(editor);
    comboBox->resize( option.rect.size() - QSize( 8, 0 ) );
    comboBox->move( option.rect.x() + 4, option.rect.y() );

    if ( m_downloadDropDownRects.contains( index ) )
    {
        editor->setGeometry( m_downloadDropDownRects.value( index ) );
    }

    if ( !comboBox->property( "shownPopup" ).toBool() )
    {
        comboBox->showPopup();
        comboBox->setProperty( "shownPopup", true );
    }
}


void
PlaylistItemDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const
{
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    const int style = index.data( PlayableProxyModel::StyleRole ).toInt();
    switch ( style )
    {
        case PlayableProxyModel::Collection:
        case PlayableProxyModel::Locker:
        case PlayableProxyModel::Detailed:
            paintDetailed( painter, option, index );
            break;
    }
}


void
PlaylistItemDelegate::paintDetailed( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QTextOption textOption( Qt::AlignVCenter | (Qt::Alignment)index.data( Qt::TextAlignmentRole ).toUInt() );
    textOption.setWrapMode( QTextOption::NoWrap );

    QStyleOptionViewItem opt = option;
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
    if ( m_view->proxyModel()->style() == PlayableProxyModel::Locker && index.column() == PlayableModel::Download )
    {
        DownloadButton::drawPrimitive( painter, opt.rect.adjusted( 4, 0, -4, 0 ), item->query(), hoveringOver() == index );
    }
    else if ( item->isPlaying() )
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

    int i = 0;
    foreach ( const Tomahawk::source_ptr& s, sources )
    {
        if ( i >= count )
            break;

        QRect r = avatarsRect.adjusted( ( avatarSize + avatarMargin ) * i, 0, 0, 0 );
        r.setWidth( avatarSize + avatarMargin );

        QPixmap pixmap = s->avatar( TomahawkUtils::Original, QSize( avatarSize, avatarSize ), true );
        painter->drawPixmap( r.adjusted( avatarMargin / 2, 0, -( avatarMargin / 2 ), 0 ), pixmap );

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
    if ( item->query()->numResults( true ) == 0 )
        return resultRect;

    const QPixmap sourceIcon = item->query()->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) );
    if ( sourceIcon.isNull() )
        return resultRect;

    painter->setOpacity( 0.8 );
    painter->drawPixmap( QRect( rect.right() - sourceIconSize, rect.center().y() - sourceIconSize / 2, sourceIcon.width(), sourceIcon.height() ), sourceIcon );
    painter->setOpacity( 1.0 );

    return resultRect;
}


QRect
PlaylistItemDelegate::drawSource( QPainter* painter, const QStyleOptionViewItem& /* option */, const QModelIndex& /* index */, const QRect& rect, PlayableItem* item ) const
{
    painter->save();
    painter->setRenderHint( QPainter::TextAntialiasing );
    painter->setRenderHint( QPainter::SmoothPixmapTransform );

    QRect avatarRect = rect.adjusted( 22, rect.height() - 48, 0, -16 );
    QRect textRect = avatarRect.adjusted( avatarRect.height() + 24, 0, -32, 0 );
    avatarRect.setWidth( avatarRect.height() );

    QPixmap avatar = item->source()->avatar( TomahawkUtils::RoundedCorners, avatarRect.size(), true ) ;
    painter->drawPixmap( avatarRect, avatar );

    QTextOption to = QTextOption( Qt::AlignVCenter );
    to.setWrapMode( QTextOption::NoWrap );
    QFont f = painter->font();
    f.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
    painter->setFont( f );

    painter->setOpacity( 0.8 );
    painter->setPen( QColor( "#000000" ) );
    painter->drawText( textRect, painter->fontMetrics().elidedText( item->source()->friendlyName(), Qt::ElideRight, textRect.width() ), to );

    painter->setOpacity( 0.15 );
    painter->setBrush( QColor( "#000000" ) );
    painter->drawRect( rect.adjusted( 0, rect.height() - 8, -32, -8 ) );

    painter->restore();

    return rect;
}


QRect
PlaylistItemDelegate::drawTrack( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& rect, PlayableItem* item ) const
{
    const track_ptr track = item->query()->track();
    const bool hasOnlineResults = ( item->query()->numResults( true ) > 0 );

    painter->save();
    painter->setRenderHint( QPainter::TextAntialiasing );

    int rightMargin = 32;
    if ( !index.parent().isValid() )
        rightMargin = 0;

    if ( option.state & QStyle::State_Selected )
    {
        painter->setPen( TomahawkStyle::SELECTION_BACKGROUND );
        painter->setBrush( TomahawkStyle::SELECTION_BACKGROUND );
        painter->drawRect( rect.adjusted( 0, 4, -rightMargin, -4 ) );
    }
    painter->setPen( TomahawkStyle::SELECTION_FOREGROUND );
    painter->setFont( m_demiBoldFont );

    QRect r = rect.adjusted( 32, 6, -32 -rightMargin, -6 );
    const int margin = 8;

    const int numberWidth = painter->fontMetrics().width( "00" ) + 32;
    const int durationWidth = painter->fontMetrics().width( "00:00" ) + 32;
    int stateWidth = 0;

    QRect numberRect = QRect( r.x(), r.y(), numberWidth, r.height() );
    QRect extraRect = QRect( r.x() + r.width() - durationWidth, r.y(), durationWidth, r.height() );
    QRect stateRect = extraRect.adjusted( 0, 0, 0, 0 );

   if ( option.state & QStyle::State_Selected || hoveringOver() == index )
    {
        int h = extraRect.height() / 3;

        if ( track->loved() )
        {
            painter->save();
            painter->setOpacity( 0.5 );
            QRect r = stateRect.adjusted( -16, extraRect.height() / 2 - h / 2, 0, 0 );
            r.setHeight( h );
            r.setWidth( r.height() );
            painter->drawPixmap( r, ImageRegistry::instance()->pixmap( RESPATH "images/love.svg", r.size() ) );
            painter->restore();

            stateWidth += r.width() + 16;
        }
    }

    QRect downloadButtonRect = stateRect.adjusted( -stateWidth -144, 6, 0, -6 );
    downloadButtonRect.setWidth( 144 );
    stateWidth += downloadButtonRect.width() + 16;
    if ( DownloadButton::drawPrimitive( painter, downloadButtonRect, item->query(), m_hoveringOverDownloadButton == index ) )
    {
        m_downloadDropDownRects[ index ] = downloadButtonRect;
    }

    const int remWidth = r.width() - numberWidth - durationWidth;

    QRect titleRect = QRect( numberRect.x() + numberRect.width(), r.y(), (double)remWidth * 0.5, r.height() );
    QRect artistRect = QRect( titleRect.x() + titleRect.width(), r.y(), (double)remWidth * 0.5, r.height() );
    if ( stateWidth > 0 )
    {
        // Make sure we don't draw over the state icons
        artistRect.setWidth( artistRect.width() - stateWidth );
    }

    // draw title
    qreal opacityCo = 1.0;
    if ( !item->query()->playable() )
        opacityCo = 0.5;

    painter->setOpacity( 1.0 * opacityCo );
    QString text = painter->fontMetrics().elidedText( track->track(), Qt::ElideRight, titleRect.width() - margin );
    painter->drawText( titleRect, text, m_centerOption );

    // draw artist
    painter->setOpacity( 0.8 * opacityCo );
    painter->setFont( m_normalFont );
    text = painter->fontMetrics().elidedText( track->artist(), Qt::ElideRight, artistRect.width() - margin );

    painter->save();
    if ( m_hoveringOverArtist == index )
    {
        QFont f = painter->font();
        f.setUnderline( true );
        painter->setFont( f );
    }
    painter->drawText( artistRect, text, m_centerOption );
    m_artistNameRects[ index ] = painter->fontMetrics().boundingRect( artistRect, Qt::AlignLeft | Qt::AlignVCenter, text );
    painter->restore();

    // draw number or source icon
    if ( ( option.state & QStyle::State_Selected || hoveringOver() == index ) && item->query()->playable() )
    {
        const int iconHeight = numberRect.size().height() / 2;
        const QRect sourceIconRect( numberRect.x(), numberRect.y() + ( numberRect.size().height() - iconHeight ) / 2, iconHeight, iconHeight );
        painter->drawPixmap( sourceIconRect, item->query()->results().first()->sourceIcon( TomahawkUtils::Original, sourceIconRect.size() ) );
    }
    else
    {
        painter->setOpacity( 0.6 * opacityCo );
        QString number = QString::number( index.row() + 1 );
        if ( number.length() < 2 )
            number = "0" + number;
        painter->drawText( numberRect, number, m_centerOption );
    }

    if ( item->isPlaying() )
    {
        if ( m_nowPlaying != index )
        {
            connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackChange() ), Qt::UniqueConnection );
            connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackChange() ), Qt::UniqueConnection );
            connect( AudioEngine::instance(), SIGNAL( timerMilliSeconds( qint64 ) ), SLOT( onAudioEngineTick( qint64 ) ), Qt::UniqueConnection );
            m_nowPlaying = QPersistentModelIndex( index );
        }

        int h = extraRect.height() / 2;
        QRect playIconRect = extraRect.adjusted( extraRect.width() - h - 8, h / 2, -8, -h / 2 );
        playIconRect.setWidth( playIconRect.height() );
        painter->drawPixmap( playIconRect, ImageRegistry::instance()->pixmap( RESPATH "images/play.svg", playIconRect.size() ) );

        double duration = (double)AudioEngine::instance()->currentTrackTotalTime();
        if ( duration <= 0 )
            duration = item->query()->track()->duration() * 1000;

        if ( duration > 0 )
        {
            painter->save();
            painter->setPen( Qt::transparent );
            painter->setBrush( QColor( "#ff004c" ));

            QRect playBar = r.adjusted( 0, r.height() + 2, 0, 0 );
            playBar.setHeight( 2 );
            painter->setOpacity( 0.1 );
            painter->drawRect( playBar );

            playBar.setWidth( ( (double)AudioEngine::instance()->currentTime() / duration ) * (double)playBar.width() );
            painter->setOpacity( 1 );
            painter->drawRect( playBar );

            painter->restore();
        }
    }
    else if ( track->duration() > 0 )
    {
        painter->setOpacity( 0.5 * opacityCo );
        painter->drawText( extraRect, TomahawkUtils::timeToString( track->duration() ), m_centerRightOption );
    }

    painter->restore();

    return r;
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

    bool hoveringArtist = false;
    bool hoveringInfo = false;
    bool hoveringLove = false;
    bool hoveringDownloadDropDown = false;
    Tomahawk::source_ptr hoveredAvatar;
    QRect hoveredAvatarRect;

    if ( m_infoButtonRects.contains( index ) )
    {
        const QRect infoRect = m_infoButtonRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringInfo = infoRect.contains( ev->pos() );
    }
    if ( m_artistNameRects.contains( index ) )
    {
        const QRect nameRect = m_artistNameRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringArtist = nameRect.contains( ev->pos() );
    }
    if ( m_loveButtonRects.contains( index ) )
    {
        const QRect loveRect = m_loveButtonRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringLove = loveRect.contains( ev->pos() );
    }
    if ( m_downloadDropDownRects.contains( index ) )
    {
        const QRect downloadDropDownRect = m_downloadDropDownRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringDownloadDropDown = downloadDropDownRect.contains( ev->pos() );
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
        if ( hoveringInfo || hoveringLove || hoveringArtist || hoveringDownloadDropDown )
            m_view->setCursor( Qt::PointingHandCursor );
        else
            m_view->setCursor( Qt::ArrowCursor );

        if ( !hoveredAvatar.isNull() )
        {
            QToolTip::showText( m_view->mapToGlobal( hoveredAvatarRect.bottomLeft() ),
                                hoveredAvatar->friendlyName(),
                                m_view,
                                hoveredAvatarRect );
        }

        if ( hoveringArtist && m_hoveringOverArtist != index )
        {
            emit updateIndex( m_hoveringOverArtist );
            emit updateIndex( index );
            m_hoveringOverArtist = index;
        }
        if ( !hoveringArtist && m_hoveringOverArtist.isValid() )
        {
            emit updateIndex( m_hoveringOverArtist );
            m_hoveringOverArtist = QModelIndex();
        }
        if ( hoveringDownloadDropDown && m_hoveringOverDownloadButton != index )
        {
            QPersistentModelIndex ti = m_hoveringOverDownloadButton;
            m_hoveringOverDownloadButton = index;

            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( ti ) );
            item->requestRepaint();
            emit updateIndex( m_hoveringOverDownloadButton );
        }
        if ( !hoveringDownloadDropDown && m_hoveringOverDownloadButton.isValid() )
        {
            QPersistentModelIndex ti = m_hoveringOverDownloadButton;
            m_hoveringOverDownloadButton = QModelIndex();

            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( ti ) );
            item->requestRepaint();
        }

        if ( m_hoveringOver != index )
        {
            emit updateIndex( m_hoveringOver );
            emit updateIndex( index );
            m_hoveringOver = index;
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

        if ( hoveringArtist )
        {
            ViewManager::instance()->show( item->query()->track()->artistPtr() );
        }
        else if ( hoveringLove )
        {
            item->query()->queryTrack()->setLoved( !item->query()->queryTrack()->loved() );
        }
        else if ( hoveringDownloadDropDown || ( m_view->proxyModel()->style() == PlayableProxyModel::Locker && index.column() == PlayableModel::Download ) )
        {
            if ( DownloadButton::handleEditorEvent( event , m_view, m_model, index ) )
                return true;
        }
        else if ( hoveringInfo )
        {
            if ( m_model->style() == PlayableProxyModel::SingleColumn )
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
    if ( !m_model || !m_hoveringOver.isValid() )
        return;

    QPersistentModelIndex idx = m_hoveringOver;

    m_hoveringOver = QModelIndex();
    m_hoveringOverArtist = QModelIndex();
    m_hoveringOverDownloadButton = QModelIndex();
    m_infoButtonRects.clear();
    m_loveButtonRects.clear();
    m_artistNameRects.clear();

    QModelIndex itemIdx = m_model->mapToSource( idx );
    if ( itemIdx.isValid() )
    {
        PlayableItem* item = m_model->sourceModel()->itemFromIndex( itemIdx );
        if ( item )
            item->requestRepaint();
    }

    emit updateIndex( idx );
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


void
PlaylistItemDelegate::onAudioEngineTick( qint64 /* ms */ )
{
    doUpdateIndex( m_nowPlaying );
}


void
PlaylistItemDelegate::onPlaybackChange()
{
    disconnect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), this, SLOT( onPlaybackChange() ) );
    disconnect( AudioEngine::instance(), SIGNAL( stopped() ), this, SLOT( onPlaybackChange() ) );
    disconnect( AudioEngine::instance(), SIGNAL( timerMilliSeconds( qint64 ) ), this, SLOT( onAudioEngineTick( qint64 ) ) );
    doUpdateIndex( m_nowPlaying );
    m_nowPlaying = QModelIndex();
}
