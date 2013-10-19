/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "PlaylistLargeItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QDateTime>

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

// Forward Declarations breaking QSharedPointer
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    #include "utils/PixmapDelegateFader.h"
#endif


using namespace Tomahawk;


PlaylistLargeItemDelegate::PlaylistLargeItemDelegate( DisplayMode mode, TrackView* parent, PlayableProxyModel* proxy )
    : PlaylistItemDelegate( parent, proxy )
    , m_view( parent )
    , m_model( proxy )
    , m_mode( mode )
{
}


QSize
PlaylistLargeItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    int rowHeight = option.fontMetrics.height() + 5;
    size.setHeight( rowHeight * 2.5 );

    return size;
}


void
PlaylistLargeItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );

    bool isUnlistened = true;
    if ( m_mode == Inbox )
    {
        QList< Tomahawk::SocialAction > socialActions = item->query()->queryTrack()->allSocialActions();
        foreach ( const Tomahawk::SocialAction& sa, socialActions )
        {
            if ( sa.action.toString() == "Inbox" && sa.value.toBool() == false )
            {
                isUnlistened = false;
                break;
            }
        }
    }

    opt.text.clear();
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( m_view->header()->visualIndex( index.column() ) > 0 )
        return;

    const track_ptr track = item->query()->track();

    //TODO: lowerText isn't displayed any more, get rid of the code path once we have an alternative
    QString lowerText;

    if ( m_mode == RecentlyPlayed && item->playbackLog().source )
    {
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->playbackLog().timestamp ), true );

        if ( item->playbackLog().source->isLocal() )
            lowerText = QString( tr( "played %1 by you", "e.g. played 3 hours ago by you" ) ).arg( playtime );
        else
            lowerText = QString( tr( "played %1 by %2", "e.g. played 3 hours ago by SomeSource" ) ).arg( playtime ).arg( item->playbackLog().source->friendlyName() );
    }

    if ( m_mode == LatestAdditions && item->query()->numResults() )
    {
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->query()->results().first()->modificationTime() ), true );
        lowerText = QString( tr( "added %1", "e.g. added 3 hours ago" ) ).arg( playtime );
    }

    if ( m_mode == LovedTracks )
        lowerText = item->query()->queryTrack()->socialActionDescription( "Love", Track::Detailed );
    else if ( m_mode == Inbox )
        lowerText = item->query()->queryTrack()->socialActionDescription( "Inbox", Track::Detailed );

    painter->save();
    {
        QRect r = opt.rect.adjusted( 4, 6, 0, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() ||
             ( m_mode == Inbox && isUnlistened ) )
        {
            const int pixMargin = 4;
            const int pixHeight = r.height() - pixMargin * 2;
            QRect npr = r.adjusted( pixMargin, pixMargin + 1, pixHeight - r.width() + pixMargin, -pixMargin + 1 );
            if ( item->isPlaying() )
            {
                painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
                r.adjust( pixHeight + 2 * pixMargin, 0, 0, 0 );
            }
            else
            {
                npr = npr.adjusted( 0, npr.height() / 4, -npr.width() / 2, -npr.height() / 4 );
                painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::InboxNewItem, TomahawkUtils::Original, npr.size() ) );
                r.adjust( npr.width() + 2 * pixMargin, 0, 0, 0 );
            }
        }

        painter->setPen( opt.palette.text().color() );

        r.adjust( 4, 0, -16, 0 );
        QRect leftRect;

        if ( hoveringOver() == index && !index.data().toString().isEmpty() && index.column() == 0 )
        {
            leftRect = drawInfoButton( painter, r, index, 0.9 );
        }
        else
        {
            leftRect = drawCover( painter, r, item, index );
        }

        leftRect.setX( leftRect.left() + 8 );
        QRect rightRect = r.adjusted( r.width() - m_smallBoldFontMetrics.width( TomahawkUtils::timeToString( track->duration() ) ), 0, 0, 0 );
        {
            const QRect leftRectBefore = leftRect;
            leftRect = drawSourceIcon( painter, leftRect, item, 0.5 );
            rightRect.moveLeft( rightRect.left() - ( leftRectBefore.width() - leftRect.width() ) );
        }

        QFont bigBoldFont = m_bigBoldFont;
        bigBoldFont.setPointSize( TomahawkUtils::defaultFontSize() + 3 );
        bigBoldFont.setWeight( 99 );

        painter->setFont( bigBoldFont );
        const QString text = painter->fontMetrics().elidedText( track->track(), Qt::ElideRight, leftRect.width() );
        painter->drawText( leftRect, text, m_topOption );

        painter->setFont( m_smallFont );
        QTextDocument textDoc;
//        if ( track->album().isEmpty() )
            textDoc.setHtml( tr( "<b>%1</b>", "e.g. by SomeArtist" ).arg( track->artist() ) );
/*        else
            textDoc.setHtml( tr( "by <b>%1</b> on <b>%2</b>", "e.g. by SomeArtist on SomeAlbum" ).arg( track->artist() ).arg( track->album() ) );*/
        textDoc.setDocumentMargin( 0 );
        textDoc.setDefaultFont( painter->font() );
        textDoc.setDefaultTextOption( m_topOption );

        if ( !( option.state & QStyle::State_Selected || item->isPlaying() ) )
        {
            QColor mid = opt.palette.mid().color();
            //HACK: adjust small text shade based on a guess if normal text is darker or lighter
            //      than normal background.
            if ( opt.palette.text().color().lightness() < opt.palette.base().color().lightness() )
                painter->setPen( mid.darker( 140 ) );
            else
                painter->setPen( mid.lighter( 140 ) );
        }

        if ( textDoc.idealWidth() <= leftRect.width() )
            drawRichText( painter, opt, leftRect.adjusted( 0, QFontMetrics( bigBoldFont ).height() + 1, 0, 0 ), Qt::AlignTop, textDoc );

        //TODO: replace usage of lowerText which is not drawn any more with appropriate loveBox/sentBox style boxes
        textDoc.setHtml( lowerText );
        textDoc.setDocumentMargin( 0 );
        textDoc.setDefaultFont( painter->font() );
        textDoc.setDefaultTextOption( m_bottomOption );

        if ( textDoc.idealWidth() > leftRect.width() )
            textDoc.setHtml( item->query()->queryTrack()->socialActionDescription( "Love", Track::Short ) );

//        drawRichText( painter, opt, leftRect, Qt::AlignBottom, textDoc );

        leftRect = rightRect.adjusted( -128, 4, 0, -4 );
        leftRect.setWidth( 96 );
        if ( m_mode == Inbox )
        {
            QDateTime earliestTimestamp = QDateTime::currentDateTime();
            QList< Tomahawk::source_ptr > sources;
            foreach ( const Tomahawk::SocialAction& sa, item->query()->queryTrack()->socialActions( "Inbox", QVariant() /*neither true nor false!*/, true ) )
            {
                QDateTime saTimestamp = QDateTime::fromTime_t( sa.timestamp.toInt() );
                if ( saTimestamp < earliestTimestamp && saTimestamp.toTime_t() > 0 )
                    earliestTimestamp = saTimestamp;

                sources << sa.source;
            }

            QString timeString = TomahawkUtils::ageToString( earliestTimestamp, true );

            drawGenericBox( painter, opt, leftRect, timeString, sources, index );
        }
        else if ( m_mode == RecentlyPlayed )
        {
            if ( item->playbackLog().source )
            {
                QList< Tomahawk::source_ptr > sources;
                sources << item->playbackLog().source;

                QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->playbackLog().timestamp ), true );

                drawGenericBox( painter, opt, leftRect, playtime, sources, index );
            }
        }
        else if ( m_mode == LatestAdditions )
        {
            if ( item->query()->numResults() )
            {
                QList< Tomahawk::source_ptr > sources;

                QString modtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->query()->results().first()->modificationTime() ), true );

                drawGenericBox( painter, opt, leftRect, modtime, sources, index );
            }
        }
        else
        {
            drawLoveBox( painter, leftRect, item, index );
        }

        if ( track->duration() > 0 )
        {
            painter->setPen( opt.palette.text().color() );
            painter->setFont( m_smallBoldFont );
            painter->drawText( rightRect, TomahawkUtils::timeToString( track->duration() ), m_centerRightOption );
        }
    }
    painter->restore();
}


void
PlaylistLargeItemDelegate::modelChanged()
{
    PlaylistItemDelegate::modelChanged();
}
