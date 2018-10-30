/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011,      Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "SourceDelegate.h"

#include "items/SourceTreeItem.h"
#include "items/SourceItem.h"
#include "items/PlaylistItems.h"
#include "items/CategoryItems.h"
#include "items/TemporaryPageItem.h"
#include "items/ScriptCollectionItem.h"
#include "items/CollectionItem.h"
#include "items/InboxItem.h"
#include "items/QueueItem.h"

#include "audio/AudioEngine.h"
#include "Source.h"
#include "TomahawkSettings.h"
#include "ActionCollection.h"
#include "ViewManager.h"
#include "ContextMenu.h"
#include "resolvers/ScriptCollection.h"
#include "network/DBSyncConnectionState.h"

#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/DpiScaler.h"
#include "utils/Logger.h"

#include <QDateTime>
#include <QMimeData>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>

#define TREEVIEW_INDENT_ADD 12

SourceDelegate::SourceDelegate( QAbstractItemView* parent )
    : QStyledItemDelegate( parent )
    , m_parent( parent )
    , m_lastClicked( -1 )
    , m_margin( TomahawkUtils::DpiScaler::scaledY( m_parent, 32 ) )
{
    m_dropMimeData = new QMimeData();
}


SourceDelegate::~SourceDelegate()
{
    delete m_dropMimeData;
}


QSize
SourceDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );

    if ( type == SourcesModel::Source || type == SourcesModel::ScriptCollection )
    {
        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        return QSize( option.rect.width(), ( colItem && colItem->source() && colItem->source()->isLocal() ) ? 0 : option.fontMetrics.height() * 3.2 );
    }
    else if ( type == SourcesModel::Divider )
    {
        return QSize( option.rect.width(), TomahawkUtils::DpiScaler::scaledY( m_parent, 6 ) );
    }
    else if ( type == SourcesModel::Group )
    {
        const int groupSpacer = index.row() > 0 ? option.fontMetrics.height() * 2.5 : option.fontMetrics.height() * 1.8;
        return QSize( option.rect.width(), option.fontMetrics.height() + groupSpacer );
    }
    else
        return QSize( option.rect.width(), option.fontMetrics.height() * 1.8 ); //QStyledItemDelegate::sizeHint( option, index ) );
}


void
SourceDelegate::paintStandardItem( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QString& count ) const
{
    QFont font = painter->font();
    font.setPointSize( TomahawkUtils::defaultFontSize() );
    painter->setFont( font );

    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    const bool upperCase = !( type == SourcesModel::StaticPlaylist ||
        type == SourcesModel::AutomaticPlaylist ||
        type == SourcesModel::Station ||
        type == SourcesModel::TemporaryPage );

    QStyleOptionViewItem opt = option;
    initStyleOption( &opt, index );
    opt.showDecorationSelected = false;

    const bool selected = ( option.state & QStyle::State_Selected ) == QStyle::State_Selected;
    const bool enabled = ( option.state & QStyle::State_Enabled ) == QStyle::State_Enabled;

    QIcon::Mode iconMode = QIcon::Normal;
    if ( !enabled )
        iconMode = QIcon::Disabled;

    QRect iconRect = opt.rect.adjusted( m_margin / 2, m_margin / 6, 0, -m_margin / 6 );
    iconRect.setWidth( iconRect.height() );
    painter->drawPixmap( iconRect, opt.icon.pixmap( iconRect.size(), iconMode ) );

    if ( selected )
    {
        QFont f = painter->font();
        f.setBold( true );
        painter->setFont( f );
    }

    int figWidth = 0;
    if ( !count.isEmpty() )
    {
        figWidth = QFontMetrics( painter->font() ).width( count );
        const QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - m_margin / 2, 0, -m_margin / 2, 0 );
        painter->drawText( figRect, count, QTextOption( Qt::AlignVCenter | Qt::AlignRight ) );
    }

    QRect textRect = opt.rect.adjusted( iconRect.width() + m_margin / 2 + m_margin / 4, 0, -m_margin - figWidth, 0 );
    const QString text = painter->fontMetrics().elidedText( upperCase ? opt.text.toUpper() : opt.text, Qt::ElideRight, textRect.width() );
    {
        QTextOption to( Qt::AlignVCenter );
        to.setWrapMode( QTextOption::NoWrap );
        painter->setPen( Qt::black );

        if ( !enabled && !selected )
        {
            painter->setOpacity( 0.4 );
        }

        painter->drawText( textRect, text, to );
    }
}


void
SourceDelegate::paintDecorations( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();

    // Paint the speaker icon next to the currently-playing playlist
    const bool playable = ( type == SourcesModel::StaticPlaylist ||
        type == SourcesModel::AutomaticPlaylist ||
        type == SourcesModel::Station ||
        type == SourcesModel::TemporaryPage ||
        type == SourcesModel::LovedTracksPage ||
        type == SourcesModel::Collection ||
        type == SourcesModel::GenericPage );
    const bool playing = ( AudioEngine::instance()->isPlaying() || AudioEngine::instance()->isPaused() );

    if ( playable && playing && item->isBeingPlayed() )
    {
        const int iconW = option.rect.height() - m_margin / 4;
        const QRect iconRect( m_margin / 4, option.rect.y() + m_margin / 8, iconW, iconW );
        const QPixmap speaker = TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeakerDark, TomahawkUtils::Original, iconRect.size() );

        painter->drawPixmap( iconRect, speaker );
    }
}


void
SourceDelegate::paintSource( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();
    painter->setPen( Qt::black );
    QFont font = painter->font();
    font.setPointSize( TomahawkUtils::defaultFontSize() );
    painter->setFont( font );

    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );

    const int iconRectVertMargin = m_margin / 4;
    const QRect iconRect = option.rect.adjusted( m_margin / 2 + m_margin / 4, iconRectVertMargin, -option.rect.width() + option.rect.height() + m_margin / 4, -iconRectVertMargin );
    QString name = index.data().toString();
    QPixmap avatar;
    int figWidth = 0;
    bool isPlaying = false;
    QString desc;
    QString tracks;

    bool shouldDrawDropHint = false;

    if ( type == SourcesModel::Source )
    {
        // If the user is about to drop a track on a peer
        QRect itemsRect = option.rect;
        QPoint cursorPos = m_parent->mapFromGlobal( QCursor::pos() );
        bool cursorInRect = itemsRect.contains( cursorPos );
        if ( cursorInRect && m_dropHoverIndex.isValid() && m_dropHoverIndex == index )
            shouldDrawDropHint = true;

        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        Q_ASSERT( colItem );
        bool status = !( !colItem->source() || !colItem->source()->isOnline() );

        if ( colItem->source() && !colItem->source()->isLocal() )
        {
            if ( status )
            {
                tracks = QString::number( colItem->source()->trackCount() );
                if ( shouldDrawDropHint )
                    figWidth = iconRect.width();
                name = colItem->source()->friendlyName();
            }

            avatar = colItem->pixmap( iconRect.size() );
            isPlaying = !colItem->source()->currentTrack().isNull();
            desc = colItem->source()->textStatus();
            if ( colItem->source().isNull() )
                desc = tr( "All available tracks" );
        }
    }
    else if ( type == SourcesModel::ScriptCollection )
    {
        ScriptCollectionItem* scItem = qobject_cast< ScriptCollectionItem* >( item );
        Q_ASSERT( scItem );

        if ( !scItem->collection().isNull() )
        {
            int trackCount = scItem->collection()->trackCount();
            if ( trackCount >= 0 )
            {
                tracks = QString::number( trackCount );
                figWidth = painter->fontMetrics().width( tracks );
            }
            name = scItem->collection()->itemName();
        }

        avatar = scItem->icon().pixmap( iconRect.size() );
        desc = qobject_cast< Tomahawk::ScriptCollection* >( scItem->collection().data() )->description();
    }

    painter->setOpacity( 1.0 );
    painter->drawPixmap( iconRect, avatar );

    QRect textRect = option.rect.adjusted( iconRect.width() + m_margin, m_margin / 6 + m_margin / 32, -figWidth - ( figWidth ? m_margin : 0 ), 0 );
    QString text = painter->fontMetrics().elidedText( name, Qt::ElideRight, textRect.width() );
    {
        QTextOption to;
        to.setWrapMode( QTextOption::NoWrap );
        painter->setOpacity( 0.7 );
        painter->drawText( textRect, text, to );
    }

    textRect = option.rect.adjusted( iconRect.width() + m_margin, option.rect.height() / 2, -figWidth - ( figWidth ? m_margin : 0 ), -m_margin / 4 );

    if ( type == SourcesModel::Source )
    {
        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        Q_ASSERT( colItem );

        bool privacyOn = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::FullyPrivate;
        if ( !colItem->source().isNull() && colItem->source()->isLocal() && privacyOn )
        {
            QRect pmRect = textRect;
            pmRect.setRight( pmRect.left() + pmRect.height() );
            ActionCollection::instance()->getAction( "togglePrivacy" )->icon().paint( painter, pmRect );
            textRect.adjust( pmRect.width() + m_margin / 8, 0, 0, 0 );
        }
        if ( ( isPlaying || ( !colItem->source().isNull() && colItem->source()->isLocal() ) ) && !shouldDrawDropHint )
        {
            // Show a listen icon
            TomahawkUtils::ImageType listenAlongPixmap = TomahawkUtils::Invalid;
            TomahawkUtils::ImageType realtimeListeningAlongPixmap = TomahawkUtils::Invalid;
            if ( index.data( SourcesModel::LatchedOnRole ).toBool() )
            {
                // Currently listening along
                listenAlongPixmap = TomahawkUtils::HeadphonesOn;
                if ( !colItem->source()->isLocal() )
                {
                    realtimeListeningAlongPixmap =
                        colItem->source()->playlistInterface()->latchMode() == Tomahawk::PlaylistModes::RealTime ?
                            TomahawkUtils::PadlockClosed : TomahawkUtils::PadlockOpen;
                }
            }
            else if ( !colItem->source()->isLocal() )
            {
                listenAlongPixmap = TomahawkUtils::HeadphonesOff;
            }

            if ( listenAlongPixmap != TomahawkUtils::Invalid )
            {
                QRect pmRect = textRect;
                pmRect.setRight( pmRect.left() + pmRect.height() );
                painter->drawPixmap( pmRect, TomahawkUtils::defaultPixmap( listenAlongPixmap, TomahawkUtils::Original, pmRect.size() ) );
                textRect.adjust( pmRect.width() + m_margin / 8, 0, 0, 0 );

                m_headphoneRects[ index ] = pmRect;
            }
            else
                m_headphoneRects.remove( index );

            if ( realtimeListeningAlongPixmap != TomahawkUtils::Invalid )
            {
                QRect pmRect = textRect;
                pmRect.setRight( pmRect.left() + pmRect.height() );
                painter->drawPixmap( pmRect, TomahawkUtils::defaultPixmap( realtimeListeningAlongPixmap, TomahawkUtils::Original, pmRect.size() ) );
                textRect.adjust( pmRect.width() + m_margin / 8, 0, 0, 0 );

                m_lockRects[ index ] = pmRect;
            }
            else
                m_lockRects.remove( index );
        }
    }

    painter->save();
    if ( m_trackHovered == index )
    {
        QFont font = painter->font();
        font.setUnderline( true );
        painter->setFont( font );
    }
    textRect.adjust( 0, 0, 0, m_margin / 16 );

    if ( shouldDrawDropHint )
    {
        desc = tr( "Drop to send tracks" );
    }

    text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() - m_margin / 4 );
    {
        QTextOption to( Qt::AlignVCenter );
        to.setWrapMode( QTextOption::NoWrap );

        painter->setOpacity( 0.4 );
        painter->drawText( textRect, text, to );
    }
    painter->restore();

    bool shouldPaintTrackCount = false;
    if ( type == SourcesModel::Source )
    {
        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        Q_ASSERT( colItem );

        if ( colItem->source() && colItem->source()->currentTrack() && colItem->source()->state() == Tomahawk::SYNCED )
            m_trackRects[ index ] = textRect.adjusted( 0, 0, -textRect.width() + painter->fontMetrics().width( text ), 0 );
        else
            m_trackRects.remove( index );
    }
    else if ( type == SourcesModel::ScriptCollection )
    {
        if ( !tracks.isEmpty() )
            shouldPaintTrackCount = true;
    }

    if ( shouldPaintTrackCount || shouldDrawDropHint )
    {
        if ( shouldDrawDropHint )
        {
            const QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - iconRectVertMargin, iconRectVertMargin, -iconRectVertMargin, -iconRectVertMargin );
            painter->drawPixmap( figRect, TomahawkUtils::defaultPixmap( TomahawkUtils::Inbox, TomahawkUtils::Original, figRect.size() ) );
        }
        else
        {
            const QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - m_margin / 2, 0, -m_margin / 2, 0 );
            painter->drawText( figRect, tracks, QTextOption( Qt::AlignVCenter | Qt::AlignRight ) );
        }
    }

    painter->restore();
}


void
SourceDelegate::paintCategory( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();

    QFont font = painter->font();
    font.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    painter->setFont( font );

    painter->setPen( Qt::black );
    painter->setOpacity( 0.5 );
    painter->drawText( option.rect.adjusted( m_margin / 2, 0, -m_margin, 0 ), index.data().toString().toUpper(), QTextOption( Qt::AlignVCenter ) );

    if ( option.state & QStyle::State_MouseOver )
    {
        QString text = tr( "Show" );
        if ( option.state & QStyle::State_Open )
            text = tr( "Hide" );

        // draw close icon
        painter->setPen( TomahawkStyle::GROUP_HEADER );
        painter->drawText( option.rect.translated( -m_margin / 4, 0 ), text, QTextOption( Qt::AlignVCenter | Qt::AlignRight ) );
    }

    painter->restore();
}


void
SourceDelegate::paintGroup( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();

    QFont font = painter->font();
    font.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    painter->setFont( font );

    painter->setPen( Qt::black );
    painter->setOpacity( 0.5 );
    painter->drawText( option.rect.adjusted( m_margin, 0, -m_margin, -m_margin / 4 ), index.data().toString().toUpper(), QTextOption( Qt::AlignBottom ) );

    if ( option.state & QStyle::State_MouseOver )
    {
        QString text = tr( "Show" );
        if ( option.state & QStyle::State_Open )
            text = tr( "Hide" );

        // draw close icon
        painter->setPen( TomahawkStyle::GROUP_HEADER );
        painter->drawText( option.rect.translated( -m_margin / 4, -m_margin / 4 ), text, QTextOption( Qt::AlignBottom | Qt::AlignRight ) );
    }

    painter->restore();
}


void
SourceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( option.rect.height() == 0 )
        return;

    QStyleOptionViewItem optIndentation = option;
    QStyleOptionViewItem opt = option;

    painter->save();
    painter->setRenderHint( QPainter::TextAntialiasing );
    painter->setRenderHint( QPainter::SmoothPixmapTransform );

    const bool selected = ( option.state & QStyle::State_Selected ) == QStyle::State_Selected;
    if ( selected )
        painter->setOpacity( 1.0 );
    else
        painter->setOpacity( 0.7 );

    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    Q_ASSERT( item );

    initStyleOption( &opt, index );
    opt.icon = QIcon();
    opt.text.clear();

    // shrink the indentations
    {
        int indentMult = 0;
        QModelIndex counter = index;
        while ( counter.parent().isValid() )
        {
            indentMult++;
            counter = counter.parent();
        }

        const int indentDelta = optIndentation.rect.x() - m_parent->viewport()->x();
        optIndentation.rect.setX( optIndentation.rect.x() - indentDelta + indentMult * TomahawkUtils::DpiScaler::scaledY( m_parent, TREEVIEW_INDENT_ADD ) );
        opt.rect.setX( 0 );
    }

    if ( type == SourcesModel::Source || type == SourcesModel::ScriptCollection )
    {
        paintSource( painter, optIndentation, index );
    }
    else if ( type == SourcesModel::Group )
    {
        paintGroup( painter, opt, index );
    }
    else if ( type == SourcesModel::Category )
    {
        paintCategory( painter, optIndentation, index );
    }
    else if ( type == SourcesModel::Divider )
    {
        const QRect middle = optIndentation.rect.adjusted( 0, m_margin / 16, 0, -m_margin / 16 );
        const QColor bgcolor = opt.palette.color( QPalette::Base );

        painter->setPen( bgcolor.darker( 120 ) );
        painter->drawLine( middle.topLeft(), middle.topRight() );
        painter->setPen( bgcolor.lighter( 120 ) );
        painter->drawLine( middle.bottomLeft(), middle.bottomRight() );
    }
    else
    {
        optIndentation.state &= ~QStyle::State_MouseOver;
        if ( !index.parent().parent().isValid() )
            optIndentation.rect.adjust( m_margin / 4, 0, 0, 0 );

        if ( type == SourcesModel::Inbox || type == SourcesModel::Queue || type == SourcesModel::Collection )
        {
            QString count;
            if ( type == SourcesModel::Inbox )
            {
                InboxItem* ii = qobject_cast< InboxItem* >( item );
                if ( ii && ii->unlistenedCount() )
                    count = QString::number( ii->unlistenedCount() );
            }
            else if ( type == SourcesModel::Queue )
            {
                QueueItem* qi = qobject_cast< QueueItem* >( item );
                if ( qi && qi->unlistenedCount() )
                    count = QString::number( qi->unlistenedCount() );
            }
            else if ( type == SourcesModel::Collection )
            {
                CollectionItem* ci = qobject_cast< CollectionItem* >( item );
                if ( ci )
                    count = QString::number( ci->trackCount() );
            }

            paintStandardItem( painter, optIndentation, index, count );
        }
        else if ( type == SourcesModel::TemporaryPage || type == SourcesModel::DeletablePage || type == SourcesModel::RemovablePage )
        {
            if ( opt.state & QStyle::State_MouseOver )
            {
                m_iconHeight = ( opt.rect.height() / 2 );
                paintStandardItem( painter, optIndentation, index );

                // draw close icon
                const QRect r( opt.rect.right() - m_margin / 8 - m_iconHeight, opt.rect.y() + ( opt.rect.height() - m_iconHeight ) / 2, m_iconHeight, m_iconHeight );
                painter->drawPixmap( r, TomahawkUtils::defaultPixmap( TomahawkUtils::ListRemove, TomahawkUtils::Original, r.size() ) );
            }
            else
                paintStandardItem( painter, optIndentation, index );
        }
        else if ( type == SourcesModel::StaticPlaylist )
        {
            paintStandardItem( painter, optIndentation, index );

            PlaylistItem* plItem = qobject_cast< PlaylistItem* >( item );
            if ( plItem->canSubscribe() && !plItem->subscribedIcon().isNull() )
            {
                const int imgWidth = optIndentation.rect.height() / 2;
                const QPixmap icon = plItem->subscribedIcon().scaled( imgWidth, imgWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation );
                const QRect subRect( optIndentation.rect.right() - m_margin / 2 - imgWidth, optIndentation.rect.top() + ( optIndentation.rect.height() - imgWidth ) / 2, imgWidth, imgWidth );
                painter->drawPixmap( subRect, icon );
            }

            if ( plItem->collaborative() )
            {
                const int imgWidth = optIndentation.rect.height() / 2;
                const QRect subRect( optIndentation.rect.left(), optIndentation.rect.top(), imgWidth, imgWidth );

                painter->drawPixmap( subRect, TomahawkUtils::defaultPixmap( TomahawkUtils::GreenDot, TomahawkUtils::Original, subRect.size() ) );
            }
        }
        else
            paintStandardItem( painter, optIndentation, index );
    }

    paintDecorations( painter, opt, index );

    painter->restore();
}


void
SourceDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    if ( type == SourcesModel::StaticPlaylist ||
         type == SourcesModel::AutomaticPlaylist ||
         type == SourcesModel::Station )
    {
        QRect newGeometry = option.rect.adjusted( m_margin / 2 + m_margin / 4, 0, 0, 0 ); //room for the icon

#ifdef Q_OS_MAC
        newGeometry.adjust( 3 * TREEVIEW_INDENT_ADD + 5, 0, 0, 0 );  //compensate for osx indentation
#else
        newGeometry.adjust( 3 * TomahawkUtils::DpiScaler::scaledY( m_parent, TREEVIEW_INDENT_ADD ), 0, 0, 0 );  //compensate for indentation
#endif
        editor->setGeometry( newGeometry );
    }
    else
        QStyledItemDelegate::updateEditorGeometry( editor, option, index );

}


bool
SourceDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    QMouseEvent* mEvent = 0;
    switch ( event->type() )
    {
//        case QEvent::MouseTrackingChange:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
            mEvent = static_cast< QMouseEvent* >( event );
        default:
            break;
    }

    bool hoveringTrack = false;
    if ( m_trackRects.contains( index ) && mEvent )
    {
        const QRect trackRect = m_trackRects[ index ];
        hoveringTrack = trackRect.contains( mEvent->pos() );

        if ( hoveringTrack )
        {
            if ( m_trackHovered != index )
            {
                m_trackHovered = index;
                QMetaObject::invokeMethod( m_parent, "update", Qt::QueuedConnection, Q_ARG( QModelIndex, index ) );
            }
        }
    }
    if ( !hoveringTrack )
    {
        if ( m_trackHovered.isValid() )
            QMetaObject::invokeMethod( m_parent, "update", Qt::QueuedConnection, Q_ARG( QModelIndex, m_trackHovered ) );

        m_trackHovered = QPersistentModelIndex();
    }

    bool lockRectContainsClick = false, headphonesRectContainsClick = false;
    if ( m_headphoneRects.contains( index ) && mEvent )
    {
        const QRect headphoneRect = m_headphoneRects[ index ];
        headphonesRectContainsClick = headphoneRect.contains( mEvent->pos() );
    }
    if ( m_lockRects.contains( index ) && mEvent )
    {
        const QRect lockRect = m_lockRects[ index ];
        lockRectContainsClick = lockRect.contains( mEvent->pos() );
    }

    if ( event->type() == QEvent::MouseMove )
    {
        if ( hoveringTrack || lockRectContainsClick || headphonesRectContainsClick )
            m_parent->setCursor( Qt::PointingHandCursor );
        else
            m_parent->setCursor( Qt::ArrowCursor );
    }

    if ( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonPress )
    {
        SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
        if ( type == SourcesModel::TemporaryPage || type == SourcesModel::DeletablePage || type == SourcesModel::RemovablePage )
        {
            SourceTreeItem* gpi = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
            Q_ASSERT( gpi );

            QStyleOptionViewItem o = option;
            initStyleOption( &o, index );
            const int padding = m_margin / 8;
            const QRect r( o.rect.right() - padding - m_iconHeight, padding + o.rect.y(), m_iconHeight, m_iconHeight );

            if ( r.contains( mEvent->pos() ) )
            {
                if ( event->type() == QEvent::MouseButtonRelease && mEvent->button() == Qt::LeftButton )
                {
                    gpi->removeFromList();

                    // Send a new mouse event to the view, since if the mouse is now over another item's delete area we want it to show up
                    QMouseEvent* ev = new QMouseEvent( QEvent::MouseMove, m_parent->viewport()->mapFromGlobal( QCursor::pos() ), Qt::NoButton, Qt::NoButton, Qt::NoModifier );
                    QApplication::postEvent( m_parent->viewport(), ev );
                }

                return true;
            }
        }
        else if ( type == SourcesModel::Source )
        {
            SourceItem* colItem = qobject_cast< SourceItem* >( index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
            Q_ASSERT( colItem );

            if ( hoveringTrack && colItem->source() && colItem->source()->currentTrack() )
            {
                if ( event->type() == QEvent::MouseButtonRelease && mEvent->button() == Qt::LeftButton )
                {
                    ViewManager::instance()->show( colItem->source()->currentTrack() );
                    return true;
                }
                else if ( event->type() == QEvent::MouseButtonPress && mEvent->button() == Qt::RightButton )
                {
                    Tomahawk::ContextMenu* contextMenu = new Tomahawk::ContextMenu( m_parent );
                    contextMenu->setQuery( colItem->source()->currentTrack() );
                    contextMenu->exec( QCursor::pos() );
                    return true;
                }
            }

            if ( !colItem->source().isNull() && !colItem->source()->currentTrack().isNull() && !colItem->source()->isLocal() )
            {
                if ( headphonesRectContainsClick || lockRectContainsClick )
                {
                    if ( event->type() == QEvent::MouseButtonRelease && mEvent->button() == Qt::LeftButton )
                    {
                        if ( headphonesRectContainsClick )
                        {
                            if ( index.data( SourcesModel::LatchedOnRole ).toBool() )
                                // unlatch
                                emit latchOff( colItem->source() );
                            else
                                emit latchOn( colItem->source() );
                        }
                        else // it's in the lock rect
                            emit toggleRealtimeLatch( colItem->source(), !index.data( SourcesModel::LatchedRealtimeRole ).toBool() );
                    }
                    return true;
                }
            }
        }
        else if ( event->type() == QEvent::MouseButtonRelease && mEvent->button() == Qt::LeftButton && type == SourcesModel::StaticPlaylist )
        {
            PlaylistItem* plItem = qobject_cast< PlaylistItem* >( index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
            Q_ASSERT( plItem );

            if ( plItem->canSubscribe() && !plItem->subscribedIcon().isNull() )
            {
                const int padding = m_margin / 16;
                const int imgWidth = option.rect.height() - 2 * padding;
                const QRect subRect( option.rect.right() - padding - imgWidth, option.rect.top() + padding, imgWidth, imgWidth );

                if ( subRect.contains( mEvent->pos() ) )
                {
                    // Toggle playlist subscription
                    plItem->setSubscribed( !plItem->subscribed() );
                }
            }
        }
    }

    // We emit our own clicked() signal instead of relying on QTreeView's, because that is fired whether or not a delegate accepts
    // a mouse press event. Since we want to swallow click events when they are on headphones other action items, here we make sure we only
    // emit if we really want to
    if ( event->type() == QEvent::MouseButtonRelease && mEvent->button() == Qt::LeftButton )
    {
        if ( m_lastClicked == -1 )
        {
            m_lastClicked = QDateTime::currentMSecsSinceEpoch();
            emit clicked( index );
        }
        else
        {
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_lastClicked;
            if ( elapsed < QApplication::doubleClickInterval() )
            {
                m_lastClicked = -1;
                emit doubleClicked( index );
            } else
            {
                m_lastClicked = QDateTime::currentMSecsSinceEpoch();
                emit clicked( index );
            }
        }
    }

    return QStyledItemDelegate::editorEvent( event, model, option, index );
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


void
SourceDelegate::hovered( const QModelIndex& index, const QMimeData* mimeData )
{
    if ( !index.isValid() )
        return;

    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    if ( type == SourcesModel::Source )
    {
        m_dropMimeData->clear();
        foreach ( const QString& mimeDataFormat, mimeData->formats() )
        {
            m_dropMimeData->setData( mimeDataFormat, mimeData->data( mimeDataFormat ) );
        }
        m_dropHoverIndex = index;
    }
}


void
SourceDelegate::dragLeaveEvent()
{
    m_dropHoverIndex = QModelIndex();
}
