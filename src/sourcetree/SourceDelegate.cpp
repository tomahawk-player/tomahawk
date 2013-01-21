/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#include "items/SourceTreeItem.h"
#include "items/SourceItem.h"
#include "items/PlaylistItems.h"
#include "items/CategoryItems.h"
#include "items/TemporaryPageItem.h"

#include "audio/AudioEngine.h"
#include "AnimationHelper.h"
#include "Source.h"
#include "TomahawkSettings.h"
#include "ActionCollection.h"
#include "ViewManager.h"
#include "ContextMenu.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"


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

    if ( type == SourcesModel::Collection )
    {
        return QSize( option.rect.width(), option.fontMetrics.height() * 3.0 );
    }
    else if ( type == SourcesModel::Divider )
    {
        return QSize( option.rect.width(), 6 );
    }
    else if ( type == SourcesModel::Group )
    {
        int groupSpacer = index.row() > 0 ? option.fontMetrics.height() * 0.6 : option.fontMetrics.height() * 0.2;
        return QSize( option.rect.width(), option.fontMetrics.height() + groupSpacer );
    }
    else if ( m_expandedMap.contains( index ) )
    {
        if ( !m_expandedMap.value( index )->initialized() )
        {
            int dropTypes = dropTypeCount( item );
            QSize originalSize = QSize( option.rect.width(), option.fontMetrics.height() * 1.4 );
            QSize targetSize = originalSize + QSize( 0, dropTypes == 0 ? 0 : 38 + option.fontMetrics.height() * 1.4 );
            m_expandedMap.value( index )->initialize( originalSize, targetSize, 300 );
            m_expandedMap.value( index )->expand();
        }
        QMetaObject::invokeMethod( m_parent, "update", Qt::QueuedConnection, Q_ARG( QModelIndex, index ) );
        return m_expandedMap.value( index )->size();
    }
    else
        return QSize( option.rect.width(), option.fontMetrics.height() * 1.4 ); //QStyledItemDelegate::sizeHint( option, index ) );
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
        int iconW = option.rect.height() - 4;
        if ( m_expandedMap.contains( index ) )
        {
            AnimationHelper* ah = m_expandedMap.value( index );
            if ( ah->initialized() )
            {
                iconW = ah->originalSize().height() - 4;
            }
        }

        QRect iconRect = QRect( 4, option.rect.y() + 2, iconW, iconW );
        QPixmap speaker = option.state & QStyle::State_Selected ?
            TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, iconRect.size() ) :
            TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeakerDark, TomahawkUtils::Original, iconRect.size() );

        painter->drawPixmap( iconRect, speaker );
    }
}


void
SourceDelegate::paintCollection( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();

    QFont normal = option.font;
    QFont bold = option.font;
    bold.setBold( true );

    QFont figFont = bold;
    figFont.setFamily( "Arial Bold" );
    figFont.setWeight( QFont::Black );
    figFont.setPointSize( normal.pointSize() - 1 );

    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    SourceItem* colItem = qobject_cast< SourceItem* >( item );
    Q_ASSERT( colItem );
    bool status = !( !colItem || colItem->source().isNull() || !colItem->source()->isOnline() );

    QString tracks;
    QString name = index.data().toString();
    int figWidth = 0;

    if ( status && colItem && !colItem->source().isNull() )
    {
        tracks = QString::number( colItem->source()->trackCount() );
        figWidth = QFontMetrics( figFont ).width( tracks );
        name = colItem->source()->friendlyName();
    }

    QRect iconRect = option.rect.adjusted( 4, 6, -option.rect.width() + option.rect.height() - 12 + 4, -6 );
    QPixmap avatar = colItem->pixmap( iconRect.size() );
    painter->drawPixmap( iconRect, avatar );

    if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
    {
        painter->setPen( option.palette.color( QPalette::HighlightedText ) );
    }

    QRect textRect = option.rect.adjusted( iconRect.width() + 8, 6, -figWidth - 28, 0 );
    if ( status || colItem->source().isNull() )
        painter->setFont( bold );
    QString text = painter->fontMetrics().elidedText( name, Qt::ElideRight, textRect.width() );
    painter->drawText( textRect, text );

    bool isPlaying = !( colItem->source()->currentTrack().isNull() );
    QString desc = colItem->source()->textStatus();
    QColor descColor = QColor( "#8d8d8d" );
    if ( colItem->source().isNull() )
        desc = tr( "All available tracks" );

    painter->setFont( normal );
    textRect = option.rect.adjusted( iconRect.width() + 8, option.rect.height() / 2, -figWidth - 24, -6 );

    bool privacyOn = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::FullyPrivate;
    if ( !colItem->source().isNull() && colItem->source()->isLocal() && privacyOn )
    {
        QRect pmRect = textRect;
        pmRect.setRight( pmRect.left() + pmRect.height() );
        ActionCollection::instance()->getAction( "togglePrivacy" )->icon().paint( painter, pmRect );
        textRect.adjust( pmRect.width() + 3, 0, 0, 0 );
    }
    if ( isPlaying || ( !colItem->source().isNull() && colItem->source()->isLocal() ) )
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

        if ( isPlaying )
            descColor = Qt::black;
    }

    if ( m_trackHovered == index )
    {
        QFont font = painter->font();
        font.setUnderline( true );
        painter->setFont( font );
    }
    textRect.adjust( 0, 0, 0, 2 );
    text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() - 8 );
    QTextOption to( Qt::AlignVCenter );
    to.setWrapMode( QTextOption::NoWrap );
    painter->setPen( descColor );
    painter->drawText( textRect, text, to );

    if ( colItem->source() && colItem->source()->currentTrack() && colItem->source()->state() == DBSyncConnection::SYNCED )
        m_trackRects[ index ] = textRect.adjusted( 0, 0, -textRect.width() + painter->fontMetrics().width( text ), 0 );
    else
        m_trackRects.remove( index );

    if ( status )
    {
        painter->setRenderHint( QPainter::Antialiasing );

        QRect figRect = option.rect.adjusted( option.rect.width() - figWidth - 13, 0, -14, -option.rect.height() + option.fontMetrics.height() * 1.1 );
        int hd = ( option.rect.height() - figRect.height() ) / 2;
        figRect.adjust( 0, hd, 0, hd );

        painter->setFont( figFont );

        QColor figColor( 167, 183, 211 );
        painter->setPen( figColor );
        painter->setBrush( figColor );

        TomahawkUtils::drawBackgroundAndNumbers( painter, tracks, figRect );
    }

    painter->restore();
}


void
SourceDelegate::paintCategory( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();

    QTextOption to( Qt::AlignVCenter );

    painter->setPen( option.palette.color( QPalette::Base ) );
    painter->setBrush( option.palette.color( QPalette::Base ) );
    painter->drawRect( option.rect );
    painter->setRenderHint( QPainter::Antialiasing );

    painter->setPen( Qt::white );
    painter->drawText( option.rect.translated( 4, 1 ), index.data().toString().toUpper(), to );
    painter->setPen( TomahawkUtils::Colors::GROUP_HEADER );
    painter->drawText( option.rect.translated( 4, 0 ), index.data().toString().toUpper(), to );

    if ( option.state & QStyle::State_MouseOver )
    {
        QString text = tr( "Show" );
        if ( option.state & QStyle::State_Open )
            text = tr( "Hide" );

        QFont font = option.font;
        font.setBold( true );
        painter->setFont( font );
        QTextOption to( Qt::AlignVCenter | Qt::AlignRight );

        // draw close icon
        painter->setPen( Qt::white );
        painter->drawText( option.rect.translated( -4, 1 ), text, to );
        painter->setPen( TomahawkUtils::Colors::GROUP_HEADER );
        painter->drawText( option.rect.translated( -4, 0 ), text, to );
    }

    painter->restore();
}


void
SourceDelegate::paintGroup( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();

    QFont font = painter->font();
    font.setPointSize( option.font.pointSize() + 1 );
    font.setBold( true );
    painter->setFont( font );

    QTextOption to( Qt::AlignBottom );

    painter->setPen( option.palette.color( QPalette::Base ) );
    painter->setBrush( option.palette.color( QPalette::Base ) );
    painter->drawRect( option.rect );
    painter->setRenderHint( QPainter::Antialiasing );

    painter->setPen( Qt::white );
    painter->drawText( option.rect.translated( 4, 1 ), index.data().toString().toUpper(), to );
    painter->setPen( TomahawkUtils::Colors::GROUP_HEADER );
    painter->drawText( option.rect.translated( 4, 0 ), index.data().toString().toUpper(), to );

    if ( option.state & QStyle::State_MouseOver )
    {
        QString text = tr( "Show" );
        if ( option.state & QStyle::State_Open )
            text = tr( "Hide" );

        QFont font = option.font;
        font.setBold( true );
        painter->setFont( font );
        QTextOption to( Qt::AlignBottom | Qt::AlignRight );

        // draw close icon
        painter->setPen( Qt::white );
        painter->drawText( option.rect.translated( -4, 1 ), text, to );
        painter->setPen( TomahawkUtils::Colors::GROUP_HEADER );
        painter->drawText( option.rect.translated( -4, 0 ), text, to );
    }

    painter->restore();
}


void
SourceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem o = option;
    QStyleOptionViewItemV4 o3 = option;

    painter->save();

    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    Q_ASSERT( item );

    if ( ( option.state & QStyle::State_Enabled ) == QStyle::State_Enabled )
    {
        o.state = QStyle::State_Enabled;

        if ( ( option.state & QStyle::State_MouseOver ) == QStyle::State_MouseOver )
        {
            o.state |= QStyle::State_MouseOver;
            o3.state |= QStyle::State_MouseOver;
        }

        if ( ( option.state & QStyle::State_Open ) == QStyle::State_Open )
        {
            o.state |= QStyle::State_Open;
        }

        if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            if ( type != SourcesModel::Collection )
                o3.state |= QStyle::State_Selected;
            else
                o3.state &= ~QStyle::State_Selected;

            o.palette.setColor( QPalette::Base, QColor( 0, 0, 0, 0 ) );
            o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
            o3.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
        }
    }

    // shrink the indentations
    {
        int indentMult = 0;
        QModelIndex counter = index;
        while ( counter.parent().isValid() )
        {
            indentMult++;
            counter = counter.parent();
        }

        int indentDelta = o.rect.x() - m_parent->viewport()->x();
        o.rect.setX( o.rect.x() - indentDelta + indentMult * TREEVIEW_INDENT_ADD );
        o3.rect.setX( 0 );
    }

    if ( type != SourcesModel::Group && type != SourcesModel::Category && type != SourcesModel::Divider )
        QApplication::style()->drawControl( QStyle::CE_ItemViewItem, &o3, painter );

    if ( type == SourcesModel::Collection )
    {
        paintCollection( painter, o, index );
    }
    else if ( ( type == SourcesModel::StaticPlaylist || type == SourcesModel::CategoryAdd ) &&
              m_expandedMap.contains( index ) && m_expandedMap.value( index )->partlyExpanded() && dropTypeCount( item ) > 0 )
    {
        // Let Qt paint the original item. We add our stuff after it
        o.state &= ~QStyle::State_Selected;
        o.showDecorationSelected = false;
        o.rect.adjust( 0, 0, 0, - option.rect.height() + m_expandedMap.value( index )->originalSize().height() );
        QStyledItemDelegate::paint( painter, o, index );

        // Get whole rect for the menu
        QRect itemsRect = option.rect.adjusted( -option.rect.x(), m_expandedMap.value( index )->originalSize().height(), 0, 0 );
        QPoint cursorPos = m_parent->mapFromGlobal( QCursor::pos() );
        bool cursorInRect = itemsRect.contains( cursorPos );

        // draw the background
        if ( m_gradient.finalStop() != itemsRect.bottomLeft() )
        {
            m_gradient = QLinearGradient( itemsRect.topLeft(), itemsRect.bottomLeft() );
            m_gradient.setColorAt( 0.0, Qt::white );
            m_gradient.setColorAt( 0.9, QColor( 0x88, 0x88, 0x88 ) );
            m_gradient.setColorAt( 1.0, QColor( 0x99, 0x99, 0x99 ) ); // dark grey
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
        fontBold.setBold( true );

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
        paintGroup( painter, o3, index );
    }
    else if ( type == SourcesModel::Category )
    {
        paintCategory( painter, o, index );
    }
    else if ( type == SourcesModel::Divider )
    {
        QRect middle = o.rect.adjusted( 0, 2, 0, -2 );
        painter->setRenderHint( QPainter::Antialiasing, false );

        QColor bgcolor = o3.palette.color( QPalette::Base );

        painter->setPen( bgcolor.darker( 120 ) );
        painter->drawLine( middle.topLeft(), middle.topRight() );
        painter->setPen( bgcolor.lighter( 120 ) );
        painter->drawLine( middle.bottomLeft(), middle.bottomRight() );
    }
    else
    {
        o.state &= ~QStyle::State_MouseOver;
        if ( !index.parent().parent().isValid() )
            o.rect.adjust( 7, 0, 0, 0 );

        if ( type == SourcesModel::TemporaryPage )
        {
            TemporaryPageItem* gpi = qobject_cast< TemporaryPageItem* >( item );
            Q_ASSERT( gpi );

            if ( gpi && o3.state & QStyle::State_MouseOver )
            {
                int padding = 3;
                m_iconHeight = ( o3.rect.height() - 2 * padding );

                o.rect.adjust( 0, 0, -( padding + m_iconHeight ), 0 );
                QStyledItemDelegate::paint( painter, o, index );

                // draw close icon
                QRect r( o3.rect.right() - padding - m_iconHeight, padding + o3.rect.y(), m_iconHeight, m_iconHeight );
                painter->drawPixmap( r, TomahawkUtils::defaultPixmap( TomahawkUtils::ListRemove, TomahawkUtils::Original, r.size() ) );
            }
            else
                QStyledItemDelegate::paint( painter, o, index );
        }
        else if ( type == SourcesModel::StaticPlaylist )
        {
            QStyledItemDelegate::paint( painter, o, index );

            PlaylistItem* plItem = qobject_cast< PlaylistItem* >( item );
            if ( plItem->canSubscribe() && !plItem->subscribedIcon().isNull() )
            {
                const int padding = 2;
                const int imgWidth = o.rect.height() - 2 * padding;

                const QPixmap icon = plItem->subscribedIcon().scaled( imgWidth, imgWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation );
                const QRect subRect( o.rect.right() - padding - imgWidth, o.rect.top() + padding, imgWidth, imgWidth );
                painter->drawPixmap( subRect, icon );
            }

            if ( plItem->collaborative() )
            {
                const int padding = 2;
                const int imgWidth = o.rect.height() - 2 * padding;
                const QRect subRect( o.rect.left(), o.rect.top(), imgWidth, imgWidth );

                painter->drawPixmap( subRect, TomahawkUtils::defaultPixmap( TomahawkUtils::GreenDot, TomahawkUtils::Original, subRect.size() ) );
            }
        }
        else
            QStyledItemDelegate::paint( painter, o, index );
    }

    paintDecorations( painter, o3, index );

    painter->restore();
}


void
SourceDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::StaticPlaylist )
        editor->setGeometry( option.rect.adjusted( 20, 0, 0, 0 ) );
    else
        QStyledItemDelegate::updateEditorGeometry( editor, option, index );

    editor->setGeometry( editor->geometry().adjusted( 2 * TREEVIEW_INDENT_ADD, 0, 0, 0 ) );
}


bool
SourceDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    bool hoveringTrack = false;
    if ( m_trackRects.contains( index ) )
    {
        const QRect trackRect = m_trackRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringTrack = trackRect.contains( ev->pos() );

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
    if ( m_headphoneRects.contains( index ) )
    {
        const QRect headphoneRect = m_headphoneRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        headphonesRectContainsClick = headphoneRect.contains( ev->pos() );
    }
    if ( m_lockRects.contains( index ) )
    {
        const QRect lockRect = m_lockRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        lockRectContainsClick = lockRect.contains( ev->pos() );
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
            TemporaryPageItem* gpi = qobject_cast< TemporaryPageItem* >( index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
            Q_ASSERT( gpi );
            QMouseEvent* ev = static_cast< QMouseEvent* >( event );

            QStyleOptionViewItemV4 o = option;
            initStyleOption( &o, index );
            int padding = 3;
            QRect r ( o.rect.right() - padding - m_iconHeight, padding + o.rect.y(), m_iconHeight, m_iconHeight );

            if ( r.contains( ev->pos() ) )
            {
                if ( event->type() == QEvent::MouseButtonRelease )
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
                const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
                if ( event->type() == QEvent::MouseButtonRelease && ev->button() == Qt::LeftButton )
                {
                    ViewManager::instance()->show( colItem->source()->currentTrack() );
                    return true;
                }
                else if ( event->type() == QEvent::MouseButtonPress && ev->button() == Qt::RightButton )
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
                    if ( event->type() == QEvent::MouseButtonRelease )
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
        else if ( event->type() == QEvent::MouseButtonRelease && type == SourcesModel::StaticPlaylist )
        {
            PlaylistItem* plItem = qobject_cast< PlaylistItem* >( index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
            Q_ASSERT( plItem );

            QMouseEvent* mev = static_cast< QMouseEvent* >( event );
            if ( plItem->canSubscribe() && !plItem->subscribedIcon().isNull() )
            {
                const int padding = 2;
                const int imgWidth = option.rect.height() - 2*padding;
                const QRect subRect( option.rect.right() - padding - imgWidth, option.rect.top() + padding, imgWidth, imgWidth );

                if ( subRect.contains( mev->pos() ) )
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
    if ( event->type() == QEvent::MouseButtonRelease )
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
    if ( !index.isValid() )
    {
        foreach ( AnimationHelper* helper, m_expandedMap )
        {
            helper->collapse();
        }
        return;
    }

    if ( !m_expandedMap.contains( index ) )
    {
        foreach ( AnimationHelper* helper, m_expandedMap )
        {
            helper->collapse();
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
}


void
SourceDelegate::animationFinished( const QModelIndex& index )
{
    delete m_expandedMap.take( index );
}
