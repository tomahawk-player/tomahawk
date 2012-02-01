/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "AccountDelegate.h"

#include <QApplication>
#include <QPainter>

#include "accounts/AccountModel.h"
#include "accounts/Account.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#define CHILD_ACCOUNT_HEIGHT 24

#define PADDING 4
#define PADDING_BETWEEN_STARS 2
#define STAR_SIZE 12

#ifdef Q_WS_MAC
#define TOPLEVEL_ACCOUNT_HEIGHT 70
#else
#define TOPLEVEL_ACCOUNT_HEIGHT 60
#endif

#define ICONSIZE 40
#define WRENCH_SIZE 24
#define STATUS_ICON_SIZE 13
#define CHECK_LEFT_EDGE 8

using namespace Tomahawk;
using namespace Accounts;

AccountDelegate::AccountDelegate( QObject* parent )
    : ConfigDelegateBase ( parent )
    , m_widestTextWidth( 0 )
{

    m_defaultCover.load( RESPATH "images/sipplugin-online.png" );
    m_ratingStarPositive.load( RESPATH "images/starred.png" );
    m_ratingStarNegative.load( RESPATH "images/star-unstarred.png" );
    m_onHoverStar.load( RESPATH "images/star-hover.png" );
    m_onlineIcon.load( RESPATH "images/sipplugin-online.png" );
    m_offlineIcon.load( RESPATH "images/sipplugin-offline.png" );

    m_ratingStarPositive = m_ratingStarPositive.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_ratingStarNegative = m_ratingStarNegative.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_onlineIcon = m_onlineIcon.scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_offlineIcon = m_offlineIcon.scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );
    m_onHoverStar = m_onHoverStar.scaled( STAR_SIZE, STAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation  );

    const int w = TOPLEVEL_ACCOUNT_HEIGHT - 2*PADDING;
    m_defaultCover = m_defaultCover.scaled( w, w, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    m_cachedIcons[ "sipplugin-online" ] = QPixmap( RESPATH "images/sipplugin-online.png" ).scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    m_cachedIcons[ "sipplugin-offline" ] = QPixmap( RESPATH "images/sipplugin-offline.png" ).scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    // save the widest width
    QFont f( QApplication::font() );
    f.setPointSize( f.pointSize() - 1 );
    QFontMetrics fm( f );
    QStringList l = QStringList() << tr( "Installed" ) << tr( "Installing" ) << tr( "Failed" ) << tr( "Uninstalling" ) << tr( "Create" );
    foreach ( const QString& str, l )
    {
        if ( fm.width( str ) > m_widestTextWidth )
            m_widestTextWidth = fm.width( str );
    }
}

bool
AccountDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    return ConfigDelegateBase::editorEvent( event, model, option, index );
}


QSize
AccountDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    AccountModel::RowType rowType = static_cast< AccountModel::RowType >( index.data( AccountModel::RowTypeRole ).toInt() );
    if ( rowType == AccountModel::TopLevelAccount || rowType == AccountModel::TopLevelFactory )
        return QSize( 200, TOPLEVEL_ACCOUNT_HEIGHT );
    else // individual child account
        return QSize( 200, CHILD_ACCOUNT_HEIGHT );
}


void
AccountDelegate::paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    painter->setRenderHint( QPainter::Antialiasing );

    AccountModel::RowType rowType = static_cast< AccountModel::RowType >( index.data( AccountModel::RowTypeRole ).toInt() );
    if ( rowType == AccountModel::TopLevelAccount || rowType == AccountModel::TopLevelFactory )
        paintTopLevel( painter, opt, index );
    else // individual child account
        paintChild( painter, opt, index );

    return;

//     const QRect itemRect = opt.rect;
//     const int top = itemRect.top();
//     const int mid = itemRect.height() / 2;
//     const int quarter = mid - ( itemRect.height() / 4 );
//
//     // one line bold for account name
//     // space below it for online/offline status
//     // checkbox, icon, name/status, features, config icon
//     QFont name = opt.font;
//     name.setPointSize( name.pointSize() + 2 );
//     name.setBold( true );
//
//     QFont smallFont = opt.font;
//     smallFont.setPointSize( smallFont.pointSize() - 1 );
//     QFontMetrics smallFontFM( smallFont );
//
//     // draw the background
//     const QWidget* w = opt.widget;
//     QStyle* style = w ? w->style() : QApplication::style();
//     style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );
/*
    int iconLeftEdge = CHECK_LEFT_EDGE + WRENCH_SIZE + PADDING;
    int textLeftEdge = iconLeftEdge + ICONSIZE + PADDING;

    // draw checkbox first
    int pos = ( mid ) - ( WRENCH_SIZE / 2 );
    QRect checkRect = QRect( CHECK_LEFT_EDGE, pos + top, WRENCH_SIZE, WRENCH_SIZE );
    opt.rect = checkRect;
    drawCheckBox( opt, painter, w );

    // draw the icon if it exists
    pos = mid - ( ICONSIZE / 2 );
    if( !index.data( Qt::DecorationRole ).value< QPixmap >().isNull() ) {
        QRect prect = QRect( iconLeftEdge, pos + top, ICONSIZE, ICONSIZE );

        painter->save();
        painter->drawPixmap( prect, index.data( Qt::DecorationRole ).value< QPixmap >().scaled( prect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
        painter->restore();
    }

    // name
    painter->save();
    painter->setFont( name );
    QFontMetrics namefm( name );
    // pos will the top-left point of the text rect
    pos = quarter - ( namefm.height() / 2 ) + top;
    const QString nameStr = index.data( AccountModel::AccountName ).toString();
    const int titleWidth = namefm.width( nameStr );
    const QRect nameRect( textLeftEdge, pos, titleWidth, namefm.height() );
    painter->drawText( nameRect, nameStr );
    painter->restore();

    // draw the online/offline status
    const int stateY = mid + quarter - ( smallFontFM.height() / 2 ) + top;

    QPixmap p;
    QString statusText;
    Account::ConnectionState state = static_cast< Account::ConnectionState >( index.data( AccountModel::ConnectionStateRole ).toInt() );
    if ( state == Account::Connected )
    {
        p = m_cachedIcons[ "sipplugin-online" ];
        statusText = tr( "Online" );
    }
    else if ( state == Account::Connecting )
    {
        p = m_cachedIcons[ "sipplugin-offline" ];
        statusText = tr( "Connecting..." );
    }
    else
    {
        p = m_cachedIcons[ "sipplugin-offline" ];
        statusText = tr( "Offline" );
    }
    painter->drawPixmap( textLeftEdge, stateY, STATUS_ICON_SIZE, STATUS_ICON_SIZE, p );

    int width = smallFontFM.width( statusText );
    int statusTextX = textLeftEdge + STATUS_ICON_SIZE + PADDING;
    painter->save();
    painter->setFont( smallFont );
    painter->drawText( QRect( statusTextX, stateY, width, smallFontFM.height() ), statusText );
    painter->restore();

    // right-most edge of text on left (name, desc) is the cutoff point for the rest of the delegate
    width = qMax( statusTextX + width, textLeftEdge + titleWidth );

    // from the right edge--config status and online/offline
    QRect confRect = QRect( itemRect.width() - WRENCH_SIZE - 2 * PADDING, mid - WRENCH_SIZE / 2 + top, WRENCH_SIZE, WRENCH_SIZE );
    if( index.data( AccountModel::HasConfig ).toBool() ) {

        QStyleOptionToolButton topt;
        topt.rect = confRect;
        topt.pos = confRect.topLeft();

        drawConfigWrench( painter, opt, topt );
    }

    const bool hasCapability = ( static_cast< Accounts::AccountTypes >( index.data( AccountModel::AccountTypeRole ).toInt() ) != Accounts::NoType );

    // draw optional capability text if it exists
    if ( hasCapability )
    {
        QString capString;
        AccountTypes types = AccountTypes( index.data( AccountModel::AccountTypeRole ).toInt() );
        if ( ( types & Accounts::SipType ) && ( types & Accounts::ResolverType ) )
            capString = tr( "Connects to, plays from friends" );
        else if ( types & Accounts::SipType )
            capString = tr( "Connects to friends" );
        else if ( types & Accounts::ResolverType )
            capString = tr( "Finds Music");

        // checkbox for capability
//         QRect capCheckRect( statusX, capY - STATUS_ICON_SIZE / 2 + top, STATUS_ICON_SIZE, STATUS_ICON_SIZE );
//         opt.rect = capCheckRect;
//         drawCheckBox( opt, painter, w );

        // text to accompany checkbox
        const int capY = mid - ( smallFontFM.height() / 2 ) + top;
        const int configLeftEdge = confRect.left() - PADDING;
        const int capW = configLeftEdge - width;
        // Right-align text
        const int capTextX = qMax( width, configLeftEdge - smallFontFM.width( capString ) );
        painter->setFont( smallFont );
        painter->drawText( QRect( capTextX, capY, configLeftEdge - capTextX, smallFontFM.height() ), Qt::AlignRight, capString );
    }*/
}


void
AccountDelegate::paintTopLevel( QPainter* painter, const QStyleOptionViewItemV4& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;

    QFont titleFont = opt.font;
    titleFont.setBold( true );
    titleFont.setPointSize( titleFont.pointSize() + 2 );
    const QFontMetrics titleMetrics( titleFont );

    QFont authorFont = opt.font;
    authorFont.setItalic( true );
    authorFont.setPointSize( authorFont.pointSize() - 1 );
    const QFontMetrics authorMetrics( authorFont );

    QFont descFont = authorFont;
    descFont.setItalic( false );
    const QFontMetrics descMetrics( descFont );

    QFont installFont = opt.font;
    installFont.setPointSize( installFont.pointSize() - 1 );
    const QFontMetrics installMetrics( descFont );

    const int height = opt.rect.height();
    const int center = height / 2 + opt.rect.top();

    // Left account enable/disable checkbox if this is not a factory
    const AccountModel::RowType rowType = static_cast< AccountModel::RowType >( index.data( AccountModel::RowTypeRole ).toInt() );
    int leftEdge = PADDING;
    if ( rowType != AccountModel::TopLevelFactory )
    {
        // draw checkbox first
        int ypos = ( center ) - ( WRENCH_SIZE / 2 );
        QRect checkRect = QRect( leftEdge, ypos, WRENCH_SIZE, WRENCH_SIZE );
        QStyleOptionViewItemV4 opt2 = opt;
        opt2.rect = checkRect;
        const AccountModel::ItemState state = static_cast< AccountModel::ItemState >( index.data( AccountModel::StateRole ).toInt() );
        const bool canCheck = ( state == AccountModel::Installed || state == AccountModel::ShippedWithTomahawk );
        opt2.state = canCheck ? QStyle::State_On : QStyle::State_Off;
        drawCheckBox( opt2, painter, opt.widget );
    }
    leftEdge += WRENCH_SIZE + PADDING / 2;

    // Pixmap
    QPixmap p = index.data( Qt::DecorationRole ).value< QPixmap >();
    const int pixmapWidth = height - 2*PADDING;
    QRect pixmapRect( leftEdge + PADDING, PADDING + opt.rect.top(), pixmapWidth, pixmapWidth );
    if ( p.isNull() ) // default image... TODO
        p = m_defaultCover;
    else
        p = p.scaled( pixmapRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

    painter->drawPixmap( pixmapRect, p );


    // Go from right edge now, stars, install/create button, downloaded info, config wrench and status etc

    // install / status button
    const AccountModel::ItemState state = static_cast< AccountModel::ItemState >( index.data( AccountModel::StateRole ).toInt() );
    QString actionText;
    switch( state )
    {
        case AccountModel::Uninstalled:
            actionText = tr( "Install" );
            break;
        case AccountModel::Installing:
            actionText = tr( "Installing" );
            break;
        case AccountModel::Upgrading:
            actionText = tr( "Upgrading" );
            break;
        case AccountModel::Failed:
            actionText = tr( "Failed" );
            break;
        case AccountModel::Installed:
            actionText = tr( "Uninstall" );
            break;
        case AccountModel::NeedsUpgrade:
            actionText = tr( "Upgrade" );
            break;
        case AccountModel::ShippedWithTomahawk:
            actionText = tr( "Create" );
            break;
        case AccountModel::UniqueFactory:
            actionText = tr( "Installed" );
            break;
    }

    // title and description
    const int btnWidth = m_widestTextWidth + 7;
    leftEdge = opt.rect.width() - PADDING - btnWidth - 3;
    const QRect btnRect( leftEdge, center - ( installMetrics.height() + 4 ) / 2, btnWidth, installMetrics.height() + 4 );
    m_cachedButtonRects[ index ] = btnRect;

    const QPen saved = painter->pen();
    painter->setPen( opt.palette.color( QPalette::Active, QPalette::AlternateBase ) );

    drawRoundedButton( painter, btnRect );

    painter->setFont( installFont );
    painter->drawText( btnRect, Qt::AlignCenter, actionText );

    painter->setPen( saved );


    // rating stars
    const int rating = index.data( AccountModel::RatingRole ).toInt();
    const int ratingWidth = 5 * ( m_ratingStarPositive.width() + PADDING_BETWEEN_STARS );
    int runningEdge = ( btnRect.right() - btnRect.width() / 2 ) - ratingWidth / 2;
    for ( int i = 1; i < 6; i++ )
    {
        QRect r( runningEdge, btnRect.top() - m_ratingStarPositive.height() - PADDING, m_ratingStarPositive.width(), m_ratingStarPositive.height() );
        if ( i == 1 )
            m_cachedStarRects[ index ] = r;

        const bool userHasRated = index.data( AccountModel::UserHasRatedRole ).toBool();
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
    QString count = tr( "%1 downloads" ).arg( index.data( AccountModel::DownloadCounterRole ).toInt() );
    const QRect countRect( btnRect.left(), btnRect.bottom() + PADDING, btnRect.width(), opt.rect.bottom() - PADDING - btnRect.bottom() );
    QFont countFont = descFont;
    countFont.setPointSize( countFont.pointSize() - 2 );
    countFont.setBold( true );
    painter->setFont( countFont );
    painter->drawText( countRect, Qt::AlignCenter | Qt::TextWordWrap, count );

    // author and version
    QString author = index.data( AccountModel::AuthorRole ).toString();
    const int authorWidth = authorMetrics.width( author );
    const int topTextLine = opt.rect.top() + PADDING;
    const QRect authorRect( btnRect.x() - 3*PADDING - authorWidth, topTextLine, authorWidth + 6, authorMetrics.height() );
    painter->setFont( authorFont );
    painter->drawText( authorRect, Qt::AlignCenter, author );

    // Disable version for now, that space is used
//     const QRect versionRect = authorRect.translated( 0, authorRect.height() );
//     QString version = index.data( AccountModel::VersionRole ).toString();
//     painter->drawText( versionRect, Qt::AlignCenter, version );

    // if this is a real resolver, show config wrench, state/status, and string
    int edgeOfRightExtras = btnRect.x();
    if ( rowType == AccountModel::TopLevelAccount )
    {
        const QRect confRect = QRect( btnRect.x() - 2*PADDING - WRENCH_SIZE, center - WRENCH_SIZE / 2, WRENCH_SIZE, WRENCH_SIZE );
        if( index.data( AccountModel::HasConfig ).toBool() ) {

            QStyleOptionToolButton topt;
            topt.rect = confRect;
            topt.pos = confRect.topLeft();

            drawConfigWrench( painter, opt, topt );
        }

        painter->save();
        painter->setFont( installFont );
        edgeOfRightExtras = drawStatus( painter, QPointF( confRect.x() - PADDING, center ), index );
        painter->restore();
    }

    // Title and description!
    // title
    QString title = index.data( Qt::DisplayRole ).toString();
    const int rightTitleEdge = authorRect.x() - PADDING;
    const int leftTitleEdge = pixmapRect.right() + PADDING;
    const QRect textRect( leftTitleEdge, topTextLine, rightTitleEdge - leftTitleEdge, center - opt.rect.top() - PADDING );
    painter->setFont( titleFont );
    painter->drawText( textRect, Qt::AlignVCenter | Qt::AlignLeft, title );

    // description
    QString desc = index.data( AccountModel::DescriptionRole ).toString();
    const int descWidth = edgeOfRightExtras - leftTitleEdge - PADDING;
    const QRect descRect( leftTitleEdge, center, descWidth, opt.rect.bottom() - center + PADDING );
    painter->setFont( descFont );
    painter->drawText( descRect, Qt::AlignLeft | Qt::TextWordWrap, desc );

    painter->drawLine( opt.rect.bottomLeft(), opt.rect.bottomRight() );
}


void
AccountDelegate::paintChild( QPainter* painter, const QStyleOptionViewItemV4& option, const QModelIndex& index ) const
{
    const int radius = 6;
    const int top = option.rect.top();
    QPainterPath outline;
    outline.moveTo( option.rect.topLeft() );

    outline.lineTo( option.rect.left(), option.rect.bottom() - radius );
    outline.quadTo( option.rect.bottomLeft(), QPointF( option.rect.left() + radius, option.rect.bottom() ) );
    outline.lineTo( option.rect.right() - radius, option.rect.bottom() );
    outline.quadTo( option.rect.bottomRight(), QPointF( option.rect.right() - 1, top ) );
    outline.lineTo( option.rect.right(), top );

    painter->drawPath( outline );

    // draw checkbox first
    const int smallWrenchSize = option.rect.height() - PADDING;
    int ypos = ( option.rect.center().y() ) - ( smallWrenchSize  / 2 );
    QRect checkRect = QRect( option.rect.left() + PADDING, ypos, smallWrenchSize, smallWrenchSize );
    QStyleOptionViewItemV4 opt2 = option;
    opt2.rect = checkRect;
    drawCheckBox( opt2, painter, opt2.widget );

    const QString username = index.data( Qt::DisplayRole ).toString();
    QFont f = option.font;
    f.setPointSize( 9 );
    painter->setFont( f );
    painter->drawText( option.rect.adjusted( PADDING + checkRect.right(), 0, 0, 0 ), Qt::AlignVCenter | Qt::AlignLeft, username );


}


void
AccountDelegate::drawRoundedButton( QPainter* painter, const QRect& btnRect ) const
{
    QPainterPath btnPath;
    const int radius = 3;
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
}


int
AccountDelegate::drawStatus( QPainter* painter, const QPointF& rightCenterEdge, const QModelIndex& index ) const
{
    QPixmap p;
    QString statusText;
    Account::ConnectionState state = static_cast< Account::ConnectionState >( index.data( AccountModel::ConnectionStateRole ).toInt() );
    if ( state == Account::Connected )
    {
        p = m_onlineIcon;
        statusText = tr( "Online" );
    }
    else if ( state == Account::Connecting )
    {
        p = m_offlineIcon;
        statusText = tr( "Connecting..." );
    }
    else
    {
        p = m_offlineIcon;
        statusText = tr( "Offline" );
    }

    const int yPos = rightCenterEdge.y() - painter->fontMetrics().height() / 2;
    const QRect connectIconRect( rightCenterEdge.x() - STATUS_ICON_SIZE, yPos, STATUS_ICON_SIZE, STATUS_ICON_SIZE );
    painter->drawPixmap( connectIconRect, p );

    int width = painter->fontMetrics().width( statusText );
    int statusTextX = connectIconRect.x() - PADDING - width;
    painter->drawText( QRect( statusTextX, yPos, width, painter->fontMetrics().height() ), statusText );

    return statusTextX;
}


QRect
AccountDelegate::checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx, int role ) const
{
//     if ( role == Qt::CheckStateRole )
//     {
//         // the whole resolver checkbox
//         QStyleOptionViewItemV4 opt = option;
//         initStyleOption( &opt, idx );
//         const int mid = opt.rect.height() / 2;
//         const int pos = mid - ( ICONSIZE / 2 );
//         QRect checkRect( CHECK_LEFT_EDGE, pos + opt.rect.top(), ICONSIZE, ICONSIZE );
//
//         return checkRect;
//     } else if ( role == AccountModel::AccountTypeRole )
//     {
//         // The capabilities checkbox
//         QStyleOptionViewItemV4 opt = option;
//         initStyleOption( &opt, idx );
//         const int quarter = opt.rect.height() / 4 + opt.rect.height()  / 2;
//         const int leftEdge = opt.rect.width() - PADDING - WRENCH_SIZE - PADDING - WRENCH_SIZE;
//         QRect checkRect( leftEdge, quarter, WRENCH_SIZE, WRENCH_SIZE );
//         return checkRect;
//     }
    return QRect();
}

QRect
AccountDelegate::configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, idx );
    QRect itemRect = opt.rect;
    QRect confRect = QRect( itemRect.width() - ICONSIZE - 2 * PADDING, (opt.rect.height() / 2) - ICONSIZE / 2 + opt.rect.top(), ICONSIZE, ICONSIZE );
    return confRect;
}

void
AccountDelegate::askedForEdit( const QModelIndex& idx )
{
    emit openConfig( qobject_cast< Account* >( idx.data( AccountModel::AccountData ).value< QObject* >() ) );
}


