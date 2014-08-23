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
#include "items/InboxItem.h"

#include "audio/AudioEngine.h"
#include "AnimationHelper.h"
#include "Source.h"
#include "TomahawkSettings.h"
#include "ActionCollection.h"
#include "ViewManager.h"
#include "ContextMenu.h"
#include "resolvers/ScriptCollection.h"
#include "network/DBSyncConnectionState.h"

#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
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
{
    m_dropTypeMap.insert( 0, SourceTreeItem::DropTypeThisTrack );
    m_dropTypeMap.insert( 1, SourceTreeItem::DropTypeThisAlbum );
    m_dropTypeMap.insert( 2, SourceTreeItem::DropTypeAllFromArtist );
    m_dropTypeMap.insert( 3, SourceTreeItem::DropTypeLocalItems );
    m_dropTypeMap.insert( 4, SourceTreeItem::DropTypeTop50 );

    m_dropTypeTextMap.insert( 0, tr( "Track" ) );
    m_dropTypeTextMap.insert( 1, tr( "Album" ) );
    m_dropTypeTextMap.insert( 2, tr( "Artist" ) );
    m_dropTypeTextMap.insert( 3, tr( "Local" ) );
    m_dropTypeTextMap.insert( 4, tr( "Top 10" ) );

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

    if ( type == SourcesModel::Collection || type == SourcesModel::ScriptCollection )
    {
        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        return QSize( option.rect.width(), ( colItem && colItem->source() && colItem->source()->isLocal() ) ? 0 : option.fontMetrics.height() * 3.0 );
    }
    else if ( type == SourcesModel::Divider )
    {
        return QSize( option.rect.width(), 6 );
    }
    else if ( type == SourcesModel::Group )
    {
        int groupSpacer = index.row() > 0 ? option.fontMetrics.height() * 2.5 : option.fontMetrics.height() * 0.8;
        return QSize( option.rect.width(), option.fontMetrics.height() + groupSpacer );
    }
    else if ( m_expandedMap.contains( index ) )
    {
        if ( !m_expandedMap.value( index )->initialized() )
        {
            int dropTypes = dropTypeCount( item );
            QSize originalSize = QSize( option.rect.width(), option.fontMetrics.height() * 1.8 );
            QSize targetSize = originalSize + QSize( 0, dropTypes == 0 ? 0 : 38 + option.fontMetrics.height() * 1.8 );
            m_expandedMap.value( index )->initialize( originalSize, targetSize, 600 );
            m_expandedMap.value( index )->expand();
        }
        QMetaObject::invokeMethod( m_parent, "update", Qt::QueuedConnection, Q_ARG( QModelIndex, index ) );
        return m_expandedMap.value( index )->size();
    }
    else
        return QSize( option.rect.width(), option.fontMetrics.height() * 1.8 ); //QStyledItemDelegate::sizeHint( option, index ) );
}


void
SourceDelegate::paintStandardItem( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    const bool upperCase = !( type == SourcesModel::StaticPlaylist ||
        type == SourcesModel::AutomaticPlaylist ||
        type == SourcesModel::Station ||
        type == SourcesModel::TemporaryPage );

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    opt.showDecorationSelected = false;

    const bool selected = ( option.state & QStyle::State_Selected ) == QStyle::State_Selected;
    const bool enabled = ( option.state & QStyle::State_Enabled ) == QStyle::State_Enabled;

    QIcon::Mode iconMode = QIcon::Normal;
    if ( !enabled )
        iconMode = QIcon::Disabled;

    QRect iconRect = opt.rect.adjusted( 14, 4, 0, -4 );
    iconRect.setWidth( iconRect.height() );
    painter->drawPixmap( iconRect, opt.icon.pixmap( iconRect.size(), iconMode ) );

    if ( selected )
    {
        QFont f = painter->font();
        f.setBold( true );
        painter->setFont( f );
    }

    QRect textRect = opt.rect.adjusted( iconRect.width() + 22, 0, -32, 0 );
    QString text = painter->fontMetrics().elidedText( upperCase ? opt.text.toUpper() : opt.text, Qt::ElideRight, textRect.width() );
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
        type == SourcesModel::GenericPage );
    const bool playing = ( AudioEngine::instance()->isPlaying() || AudioEngine::instance()->isPaused() );

    if ( playable && playing && item->isBeingPlayed() )
    {
        int iconW = option.rect.height() - 8;
        if ( m_expandedMap.contains( index ) )
        {
            AnimationHelper* ah = m_expandedMap.value( index );
            if ( ah->initialized() )
            {
                iconW = ah->originalSize().height() - 8;
            }
        }

        QRect iconRect = QRect( 8, option.rect.y() + 4, iconW, iconW );
        QPixmap speaker = TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeakerDark, TomahawkUtils::Original, iconRect.size() );

        painter->drawPixmap( iconRect, speaker );
    }
}


void
SourceDelegate::paintCollection( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();
    painter->setPen( Qt::black );

    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );

    const int iconRectVertMargin = 6;
    const QRect iconRect = option.rect.adjusted( 20, iconRectVertMargin, -option.rect.width() + option.rect.height() - 12 + 20, -iconRectVertMargin );
    QString name = index.data().toString();
    QPixmap avatar;
    int figWidth = 0;
    bool isPlaying = false;
    QString desc;
    QString tracks;

    bool shouldDrawDropHint = false;

    if ( type == SourcesModel::Collection )
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
                figWidth = painter->fontMetrics().width( tracks );
                if ( shouldDrawDropHint )
                    figWidth = iconRect.width();
                name = colItem->source()->friendlyName();
            }

            avatar = colItem->pixmap( iconRect.size() );

            isPlaying = !( colItem->source()->currentTrack().isNull() );
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

    QRect textRect = option.rect.adjusted( iconRect.width() + 28, 6, -figWidth - ( figWidth ? 28 : 0 ), 0 );
    QString text = painter->fontMetrics().elidedText( name, Qt::ElideRight, textRect.width() );
    {
        QTextOption to;
        to.setWrapMode( QTextOption::NoWrap );
        painter->setOpacity( 0.7 );
        painter->drawText( textRect, text, to );
    }

    textRect = option.rect.adjusted( iconRect.width() + 28, option.rect.height() / 2, -figWidth - ( figWidth ? 24 : 0 ), -6 );

    if ( type == SourcesModel::Collection )
    {
        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        Q_ASSERT( colItem );

        bool privacyOn = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::FullyPrivate;
        if ( !colItem->source().isNull() && colItem->source()->isLocal() && privacyOn )
        {
            QRect pmRect = textRect;
            pmRect.setRight( pmRect.left() + pmRect.height() );
            ActionCollection::instance()->getAction( "togglePrivacy" )->icon().paint( painter, pmRect );
            textRect.adjust( pmRect.width() + 3, 0, 0, 0 );
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
                textRect.adjust( pmRect.width() + 3, 0, 0, 0 );

                m_headphoneRects[ index ] = pmRect;
            }
            else
                m_headphoneRects.remove( index );

            if ( realtimeListeningAlongPixmap != TomahawkUtils::Invalid )
            {
                QRect pmRect = textRect;
                pmRect.setRight( pmRect.left() + pmRect.height() );
                painter->drawPixmap( pmRect, TomahawkUtils::defaultPixmap( realtimeListeningAlongPixmap, TomahawkUtils::Original, pmRect.size() ) );
                textRect.adjust( pmRect.width() + 3, 0, 0, 0 );

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
    textRect.adjust( 0, 0, 0, 2 );

    if ( shouldDrawDropHint )
    {
        desc = tr( "Drop to send tracks" );
    }

    text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() - 8 );
    {
        QTextOption to( Qt::AlignVCenter );
        to.setWrapMode( QTextOption::NoWrap );

        painter->setOpacity( 0.4 );
        painter->drawText( textRect, text, to );
    }
    painter->restore();

    bool shouldPaintTrackCount = false;
    if ( type == SourcesModel::Collection )
    {
        SourceItem* colItem = qobject_cast< SourceItem* >( item );
        Q_ASSERT( colItem );
        bool status = !( !colItem || colItem->source().isNull() || !colItem->source()->isOnline() );

        if ( colItem->source() && colItem->source()->currentTrack() && colItem->source()->state() == Tomahawk::SYNCED )
            m_trackRects[ index ] = textRect.adjusted( 0, 0, -textRect.width() + painter->fontMetrics().width( text ), 0 );
        else
            m_trackRects.remove( index );
        if ( status && !tracks.isEmpty() )
            shouldPaintTrackCount = true;
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
            QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - iconRectVertMargin, iconRectVertMargin, -iconRectVertMargin, -iconRectVertMargin );
            painter->drawPixmap( figRect, TomahawkUtils::defaultPixmap( TomahawkUtils::Inbox, TomahawkUtils::Original, figRect.size() ) );
        }
        else
        {
            QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - 16, 0, -14, -option.rect.height() + option.fontMetrics.height() * 1.1 );
            int hd = ( option.rect.height() - figRect.height() ) / 2;
            figRect.adjust( 0, hd, 0, hd );
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
    font.setPointSize( 9 );
    painter->setFont( font );

    QTextOption to( Qt::AlignVCenter );

    painter->setPen( Qt::black );
    painter->setOpacity( 0.5 );
    painter->drawText( option.rect.translated( 16, 0 ), index.data().toString().toUpper(), to );

    if ( option.state & QStyle::State_MouseOver )
    {
        QString text = tr( "Show" );
        if ( option.state & QStyle::State_Open )
            text = tr( "Hide" );

        QFont font = option.font;
        painter->setFont( font );
        QTextOption to( Qt::AlignVCenter | Qt::AlignRight );

        // draw close icon
        painter->setPen( TomahawkStyle::GROUP_HEADER );
        painter->drawText( option.rect.translated( -4, 0 ), text, to );
    }

    painter->restore();
}


void
SourceDelegate::paintGroup( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();

    QFont font = painter->font();
    font.setPointSize( 9 );
    painter->setFont( font );

    QTextOption to( Qt::AlignBottom );

    painter->setPen( Qt::black );
    painter->setOpacity( 0.5 );
    painter->drawText( option.rect.adjusted( 32, 0, -32, -8 ), index.data().toString().toUpper(), to );

    if ( option.state & QStyle::State_MouseOver )
    {
        QString text = tr( "Show" );
        if ( option.state & QStyle::State_Open )
            text = tr( "Hide" );

        QFont font = option.font;
        painter->setFont( font );
        QTextOption to( Qt::AlignBottom | Qt::AlignRight );

        // draw close icon
        painter->setPen( TomahawkStyle::GROUP_HEADER );
        painter->drawText( option.rect.translated( -4, -6 ), text, to );
    }

    painter->restore();
}


void
SourceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 optIndentation = option;
    QStyleOptionViewItemV4 opt = option;

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

        int indentDelta = optIndentation.rect.x() - m_parent->viewport()->x();
        optIndentation.rect.setX( optIndentation.rect.x() - indentDelta + indentMult * TREEVIEW_INDENT_ADD );
        opt.rect.setX( 0 );
    }

    if ( type == SourcesModel::Collection || type == SourcesModel::ScriptCollection )
    {
        paintCollection( painter, optIndentation, index );
    }
    else if ( ( type == SourcesModel::StaticPlaylist || type == SourcesModel::CategoryAdd ) &&
              m_expandedMap.contains( index ) && m_expandedMap.value( index )->partlyExpanded() && dropTypeCount( item ) > 0 )
    {
        optIndentation.rect.adjust( 0, 0, 0, - option.rect.height() + m_expandedMap.value( index )->originalSize().height() );
        paintStandardItem( painter, optIndentation, index );

        // Get whole rect for the menu
        QRect itemsRect = option.rect.adjusted( -option.rect.x(), m_expandedMap.value( index )->originalSize().height(), 0, 0 );
        QPoint cursorPos = m_parent->mapFromGlobal( QCursor::pos() );
        bool cursorInRect = itemsRect.contains( cursorPos );

        // draw the background
        if ( m_gradient.finalStop() != itemsRect.bottomLeft() )
        {
            m_gradient = QLinearGradient( itemsRect.topLeft(), itemsRect.bottomLeft() );
            m_gradient.setColorAt( 0.0, TomahawkStyle::SIDEBAR_LAZYLIST_UPPER );
            m_gradient.setColorAt( 0.9, TomahawkStyle::SIDEBAR_LAZYLIST_LOWER );
            m_gradient.setColorAt( 1.0, TomahawkStyle::SIDEBAR_LAZYLIST_LOWEST );
        }

        QPen pen = painter->pen();
        painter->setPen( QPen( Qt::NoPen ) );
        painter->setBrush( m_gradient );
        painter->drawRect( itemsRect );

        // calculate sizes for the icons
        int totalCount = dropTypeCount( item );
        int itemWidth = itemsRect.width() / totalCount;
        int iconSpacing = ( itemWidth - 32 ) / 2;

        // adjust to one single entry
        itemsRect.adjust( 0, 0, -itemsRect.width() + itemWidth, 0 );

        pen.setColor( Qt::white );
        painter->setPen( pen );

        QFont font = painter->font();
        font.setPointSize( option.font.pointSize() - 1 );
        painter->setFont( font );
        QFont fontBold = painter->font();

        QRect textRect;
        QRect imageRect;
        SourceTreeItem::DropTypes dropTypes = item->supportedDropTypes( m_dropMimeData );

        int count = 0;
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

                QPixmap pixmap;
                switch ( i )
                {
                    case 0:
                        pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DropSong, TomahawkUtils::Original, imageRect.size() );
                        break;
                    case 1:
                        pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DropAlbum, TomahawkUtils::Original, imageRect.size() );
                        break;
                    case 2:
                        pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DropAllSongs, TomahawkUtils::Original, imageRect.size() );
                        break;
                    case 3:
                        pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DropLocalSongs, TomahawkUtils::Original, imageRect.size() );
                        break;
                    case 4:
                        pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DropTopSongs, TomahawkUtils::Original, imageRect.size() );
                        break;
                }

                painter->drawPixmap( imageRect, pixmap );
            }

            count++;
        }
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
        QRect middle = optIndentation.rect.adjusted( 0, 2, 0, -2 );

        QColor bgcolor = opt.palette.color( QPalette::Base );

        painter->setPen( bgcolor.darker( 120 ) );
        painter->drawLine( middle.topLeft(), middle.topRight() );
        painter->setPen( bgcolor.lighter( 120 ) );
        painter->drawLine( middle.bottomLeft(), middle.bottomRight() );
    }
    else
    {
        optIndentation.state &= ~QStyle::State_MouseOver;
        if ( !index.parent().parent().isValid() )
            optIndentation.rect.adjust( 7, 0, 0, 0 );

        if ( type == SourcesModel::Inbox )
        {
            InboxItem* ii = qobject_cast< InboxItem* >( item );
            if ( ii && ii->unlistenedCount() )
            {
                const QString count = QString::number( ii->unlistenedCount() );
                int figWidth = QFontMetrics( painter->font() ).width( count );
                QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - 16, 0, -14, -option.rect.height() + option.fontMetrics.height() * 1.1 );
                int hd = ( option.rect.height() - figRect.height() ) / 2;
                figRect.adjust( 0, hd, 0, hd );
                painter->drawText( figRect, count, QTextOption( Qt::AlignVCenter | Qt::AlignRight ) );
            }
            paintStandardItem( painter, optIndentation, index );
        }
        else if ( type == SourcesModel::TemporaryPage )
        {
            if ( opt.state & QStyle::State_MouseOver )
            {
                m_iconHeight = ( opt.rect.height() / 2 );

                optIndentation.rect.adjust( 0, 0, -( 4 + m_iconHeight ), 0 );
                paintStandardItem( painter, optIndentation, index );

                // draw close icon
                QRect r( opt.rect.right() - 4 - m_iconHeight, opt.rect.y() + ( opt.rect.height() - m_iconHeight ) / 2, m_iconHeight, m_iconHeight );
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
                const QRect subRect( optIndentation.rect.right() - 4 - imgWidth, optIndentation.rect.top() + ( optIndentation.rect.height() - imgWidth ) / 2, imgWidth, imgWidth );
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
        QRect newGeometry = option.rect.adjusted( 20, 0, 0, 0 ); //room for the icon

#ifdef Q_OS_MAC
        newGeometry.adjust( 3 * TREEVIEW_INDENT_ADD + 5, 0, 0, 0 );  //compensate for osx indentation
#else
        newGeometry.adjust( 3 * TREEVIEW_INDENT_ADD, 0, 0, 0 );  //compensate for indentation
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
        if ( type == SourcesModel::TemporaryPage )
        {
            SourceTreeItem* gpi = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
            Q_ASSERT( gpi );

            QStyleOptionViewItemV4 o = option;
            initStyleOption( &o, index );
            int padding = 3;
            QRect r ( o.rect.right() - padding - m_iconHeight, padding + o.rect.y(), m_iconHeight, m_iconHeight );

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
        else if ( type == SourcesModel::Collection )
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
                const int padding = 2;
                const int imgWidth = option.rect.height() - 2*padding;
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


SourceTreeItem::DropType
SourceDelegate::hoveredDropType() const
{
    return m_hoveredDropType;
}


void
SourceDelegate::hovered( const QModelIndex& index, const QMimeData* mimeData )
{
    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );

    if ( !index.isValid() )
    {
        foreach ( AnimationHelper* helper, m_expandedMap )
        {
            helper->collapse( true );
        }
        return;
    }
/*    if ( ( type == SourcesModel::StaticPlaylist || type == SourcesModel::CategoryAdd ) &&
         !m_expandedMap.contains( index ) )
    {
        foreach ( AnimationHelper* helper, m_expandedMap )
        {
            helper->collapse( true );
        }

        m_newDropHoverIndex = index;
        m_dropMimeData->clear();
        foreach ( const QString& mimeDataFormat, mimeData->formats() )
        {
            m_dropMimeData->setData( mimeDataFormat, mimeData->data( mimeDataFormat ) );
        }

        m_expandedMap.insert( m_newDropHoverIndex, new AnimationHelper( m_newDropHoverIndex ) );
        connect( m_expandedMap.value( m_newDropHoverIndex ), SIGNAL( finished( QModelIndex ) ), SLOT( animationFinished( QModelIndex ) ) );
    }
    else*/ if ( type == SourcesModel::Collection )
    {
        m_dropMimeData->clear();
        foreach ( const QString& mimeDataFormat, mimeData->formats() )
        {
            m_dropMimeData->setData( mimeDataFormat, mimeData->data( mimeDataFormat ) );
        }
        m_dropHoverIndex = index;
    }
    else
        qDebug() << "expandedMap already contains index" << index;
}


void
SourceDelegate::dragLeaveEvent()
{
    foreach ( AnimationHelper* helper, m_expandedMap )
    {
        helper->collapse( true );
    }
    m_dropHoverIndex = QModelIndex();
}


void
SourceDelegate::animationFinished( const QModelIndex& index )
{
    delete m_expandedMap.take( index );
}
