/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "GetNewStuffDelegate.h"

#include "GetNewStuffModel.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include <QtGui/QPainter>
#include <QApplication>
#include <QMouseEvent>
#include "AtticaManager.h"

#define PADDING 4
#define PADDING_BETWEEN_STARS 2
#define STAR_SIZE 12

#ifdef Q_WS_MAC
#define SIZEHINT_HEIGHT 70
#else
#define SIZEHINT_HEIGHT 60
#endif

GetNewStuffDelegate::GetNewStuffDelegate( QObject* parent )
    : QStyledItemDelegate ( parent )
    , m_widestTextWidth( 0 )
    , m_hoveringOver( -1 )
{
    m_defaultCover.load( RESPATH "images/sipplugin-online.png" );
    m_ratingStarPositive.load( RESPATH "images/starred.png" );
    m_ratingStarNegative.load( RESPATH "images/star-unstarred.png" );
    m_onHoverStar.load( RESPATH "images/star-hover.png" );

    m_ratingStarPositive = m_ratingStarPositive.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_ratingStarNegative = m_ratingStarNegative.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_onHoverStar = m_onHoverStar.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );

    const int w = SIZEHINT_HEIGHT - 2*PADDING;
    m_defaultCover = m_defaultCover.scaled( w, w, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    // save the widest wifth
    QFont f( QApplication::font() );
    f.setPointSize( f.pointSize() - 1 );
    QFontMetrics fm( f );
    QStringList l = QStringList() << tr( "Installed" ) << tr( "Installing" ) << tr( "Failed" ) << tr( "Uninstalling" );
    foreach ( const QString& str, l )
    {
        if ( fm.width( str ) > m_widestTextWidth )
            m_widestTextWidth = fm.width( str );
    }
}

void
GetNewStuffDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget );

    painter->setRenderHint( QPainter::Antialiasing );

    QFont titleFont = opt.font;
    titleFont.setBold( true );
    titleFont.setPointSize( titleFont.pointSize() + 2 );
    QFontMetrics titleMetrics( titleFont );

    QFont authorFont = opt.font;
    authorFont.setItalic( true );
    authorFont.setPointSize( authorFont.pointSize() - 1 );
    QFontMetrics authorMetrics( authorFont );

    QFont descFont = authorFont;
    descFont.setItalic( false );
    QFontMetrics descMetrics( descFont );

    QFont installFont = opt.font;
    installFont.setPointSize( installFont.pointSize() - 1 );
    QFontMetrics installMetrics( descFont );

    const int height = opt.rect.height();
    const int center = height / 2 + opt.rect.top();

    // Pixmap
    QPixmap p = index.data( Qt::DecorationRole ).value< QPixmap >();
    const int pixmapWidth = height - 2*PADDING;
    QRect pixmapRect( PADDING, PADDING + opt.rect.top(), pixmapWidth, pixmapWidth );
    if ( p.isNull() ) // default image... TODO
        p = m_defaultCover;
    else
        p = p.scaled( pixmapRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

    painter->drawPixmap( pixmapRect, p );

    // Go from right edge now, stars, install button, and downloaded info

    // install / status button
    AtticaManager::ResolverState state = static_cast< AtticaManager::ResolverState >( index.data( GetNewStuffModel::StateRole ).toInt() );
    QString actionText;
    switch( state )
    {
        case AtticaManager::Uninstalled:
            actionText = tr( "Install" );
            break;
        case AtticaManager::Installing:
            actionText = tr( "Installing" );
            break;
        case AtticaManager::Upgrading:
            actionText = tr( "Upgrading" );
            break;
        case AtticaManager::Failed:
            actionText = tr( "Failed" );
            break;
        case AtticaManager::Installed:
            actionText = tr( "Uninstall" );
            break;
        case AtticaManager::NeedsUpgrade:
            actionText = tr( "Upgrade" );
            break;
    }

    const int btnWidth = m_widestTextWidth + 7;
    const int leftEdge = opt.rect.width() - PADDING - btnWidth - 3;
    const QRect btnRect( leftEdge, center - ( installMetrics.height() + 4 ) / 2, btnWidth, installMetrics.height() + 4 );
    m_cachedButtonRects[ QPair<int, int>(index.row(), index.column()) ] = btnRect;

    QPen saved = painter->pen();
    painter->setPen( opt.palette.color( QPalette::Active, QPalette::AlternateBase ) );

    QPainterPath btnPath;
    const int radius = 3;
    //btnPath.addRoundedRect( btnRect, 3, 3 );
    // draw top half gradient
    const int btnCenter = btnRect.bottom() - ( btnRect.height() / 2 );
    btnPath.moveTo( btnRect.left(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnRect.top() + radius );
    btnPath.quadTo( QPoint( btnRect.topLeft() ), QPoint( btnRect.left() + radius, btnRect.top() ) );
    btnPath.lineTo( btnRect.right() - radius, btnRect.top() );
    btnPath.quadTo( QPoint( btnRect.topRight() ), QPoint( btnRect.right(), btnRect.top() + radius ) );
    btnPath.lineTo( btnRect.right(),btnCenter );
    btnPath.lineTo( btnRect.left(), btnCenter );

    QLinearGradient g;
    g.setColorAt( 0, QColor(54, 127, 211) );
    g.setColorAt( 0.5, QColor(43, 104, 182) );
    //painter->setPen( bg.darker() );
    painter->fillPath( btnPath, g );
    //painter->drawPath( btnPath );

    btnPath = QPainterPath();
    btnPath.moveTo( btnRect.left(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnRect.bottom() - radius );
    btnPath.quadTo( QPoint( btnRect.bottomLeft() ), QPoint( btnRect.left() + radius, btnRect.bottom() ) );
    btnPath.lineTo( btnRect.right() - radius, btnRect.bottom() );
    btnPath.quadTo( QPoint( btnRect.bottomRight() ), QPoint( btnRect.right(), btnRect.bottom() - radius ) );
    btnPath.lineTo( btnRect.right(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnCenter );

    g.setColorAt( 0, QColor(34, 85, 159) );
    g.setColorAt( 0.5, QColor(35, 79, 147) );
    painter->fillPath( btnPath, g );

    painter->setFont( installFont );
    painter->drawText( btnRect, Qt::AlignCenter, actionText );

    painter->setPen( saved );

    // rating stars
    int rating = index.data( GetNewStuffModel::RatingRole ).toInt();
    const int ratingWidth = 5 * ( m_ratingStarPositive.width() + PADDING_BETWEEN_STARS );
    int runningEdge = ( btnRect.right() - btnRect.width() / 2 ) - ratingWidth / 2;
    for ( int i = 1; i < 6; i++ )
    {
        QRect r( runningEdge, btnRect.top() - m_ratingStarPositive.height() - PADDING, m_ratingStarPositive.width(), m_ratingStarPositive.height() );
        if ( i == 1 )
            m_cachedStarRects[ QPair<int, int>(index.row(), index.column()) ] = r;

        const bool userHasRated = index.data( GetNewStuffModel::UserHasRatedRole ).toBool();
        if ( !userHasRated && // Show on-hover animation if the user hasn't rated it yet, and is hovering over it
             m_hoveringOver > -1 &&
             m_hoveringItem == index )
        {
            if ( i <= m_hoveringOver ) // positive star
                painter->drawPixmap( r, m_onHoverStar );
            else
                painter->drawPixmap( r, m_ratingStarNegative );
        }
        else
        {
            if ( i <= rating ) // positive or rated star
            {
                if ( userHasRated )
                    painter->drawPixmap( r, m_onHoverStar );
                else
                    painter->drawPixmap( r, m_ratingStarPositive );
            }
            else
                painter->drawPixmap( r, m_ratingStarNegative );
        }
        runningEdge += m_ratingStarPositive.width() + PADDING_BETWEEN_STARS;
    }

    // downloaded num times, underneath button
    QString count = tr( "%1 downloads" ).arg( index.data( GetNewStuffModel::DownloadCounterRole ).toInt() );
    const QRect countRect( btnRect.left(), btnRect.bottom() + PADDING, btnRect.width(), opt.rect.bottom() - PADDING - btnRect.bottom() );
    QFont countFont = descFont;
    countFont.setPointSize( countFont.pointSize() - 2 );
    countFont.setBold( true );
    painter->setFont( countFont );
    painter->drawText( countRect, Qt::AlignCenter | Qt::TextWordWrap, count );

    // author and version
    QString author = index.data( GetNewStuffModel::AuthorRole ).toString();
    const int authorWidth = authorMetrics.width( author );
    const int topTextLine = opt.rect.top() + PADDING;
    const QRect authorRect( btnRect.x() - 3*PADDING - authorWidth, topTextLine, authorWidth + 6, authorMetrics.height() );
    painter->setFont( authorFont );
    painter->drawText( authorRect, Qt::AlignCenter, author );

    const QRect versionRect = authorRect.translated( 0, authorRect.height() );
    QString version = index.data( GetNewStuffModel::VersionRole ).toString();
    painter->drawText( versionRect, Qt::AlignCenter, version );

    // title
    QString title = index.data( Qt::DisplayRole ).toString();
    const int rightTitleEdge = authorRect.left() - PADDING;
    const int leftTitleEdge = pixmapRect.right() + PADDING;
    const QRect textRect( leftTitleEdge, topTextLine, rightTitleEdge - leftTitleEdge, versionRect.bottom() - opt.rect.top() - PADDING );
    painter->setFont( titleFont );
    painter->drawText( textRect, Qt::AlignVCenter | Qt::AlignLeft, title );

    // description
    QString desc = index.data( GetNewStuffModel::DescriptionRole ).toString();
    const int descWidth = btnRect.left() - leftTitleEdge - PADDING;
    const QRect descRect( leftTitleEdge, versionRect.bottom(), descWidth, opt.rect.bottom() - versionRect.bottom() + PADDING );
    painter->setFont( descFont );
    painter->drawText( descRect, Qt::AlignLeft | Qt::TextWordWrap, desc );
}


QSize
GetNewStuffDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );
    return QSize( 200, SIZEHINT_HEIGHT );
}

bool
GetNewStuffDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( option );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove )
        return false;

    if ( event->type() == QEvent::MouseButtonRelease && m_cachedButtonRects.contains( QPair<int, int>( index.row(), index.column() ) ) )
    {
        QRect rect = m_cachedButtonRects[ QPair<int, int>( index.row(), index.column() ) ];
        QMouseEvent* me = static_cast< QMouseEvent* >( event );

        if ( rect.contains( me->pos() ) )
        {
            model->setData( index, true );

            return true;
        }
    }

    if ( m_cachedStarRects.contains( QPair<int, int>( index.row(), index.column() ) ) )
    {
        QRect fullStars = m_cachedStarRects[ QPair<int, int>( index.row(), index.column() ) ];
        const int starsWidth = 5 * ( m_ratingStarPositive.width() + PADDING_BETWEEN_STARS );
        fullStars.setWidth( starsWidth );

        QMouseEvent* me = static_cast< QMouseEvent* >( event );

        if ( fullStars.contains( me->pos() ) )
        {
            const int eachStar = starsWidth / 5;
            const int clickOffset = me->pos().x() - fullStars.x();
            const int whichStar = (clickOffset / eachStar) + 1;

            if ( event->type() == QEvent::MouseButtonRelease )
            {
                model->setData( index, whichStar, GetNewStuffModel::RatingRole );
            }
            else if ( event->type() == QEvent::MouseMove )
            {
                // 0-indexed
                m_hoveringOver = whichStar;
                m_hoveringItem = index;
            }

            return true;
        }
    }

    if ( m_hoveringOver > -1 )
    {
        emit update( m_hoveringItem );
        m_hoveringOver = -1;
        m_hoveringItem = QPersistentModelIndex();
    }
    return false;
}
