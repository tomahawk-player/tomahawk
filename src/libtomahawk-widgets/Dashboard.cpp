/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Dashboard.h"
#include "ui_Dashboard.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "widgets/RecentPlaylistsModel.h"
#include "widgets/RecentlyPlayedPlaylistsModel.h"
#include "MetaPlaylistInterface.h"
#include "PlaylistDelegate.h"
#include "audio/AudioEngine.h"
#include "playlist/AlbumModel.h"
#include "playlist/RecentlyPlayedModel.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "widgets/OverlayWidget.h"
#include "widgets/BasicHeader.h"
#include "utils/ImageRegistry.h"
#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPainter>
#include <QScrollArea>

#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_TRACK_ITEMS 15

using namespace Tomahawk;
using namespace Tomahawk::Widgets;


Dashboard::Dashboard( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::Dashboard )
{
    QWidget* widget = new QWidget;
    ui->setupUi( widget );

    ui->lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    ui->lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );

    {
        m_tracksModel = new RecentlyPlayedModel( ui->tracksView, HISTORY_TRACK_ITEMS );
        ui->tracksView->proxyModel()->setStyle( PlayableProxyModel::ShortWithAvatars );
        ui->tracksView->overlay()->setEnabled( false );
        ui->tracksView->setPlaylistModel( m_tracksModel );
        ui->tracksView->setAutoResize( true );
        ui->tracksView->setAlternatingRowColors( false );
        m_tracksModel->setSource( source_ptr() );

        QPalette p = ui->tracksView->palette();
        p.setColor( QPalette::Text, TomahawkStyle::PAGE_TRACKLIST_TRACK_SOLVED );
        p.setColor( QPalette::BrightText, TomahawkStyle::PAGE_TRACKLIST_TRACK_UNRESOLVED );
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_TRACKLIST_NUMBER );
        p.setColor( QPalette::Highlight, TomahawkStyle::PAGE_TRACKLIST_HIGHLIGHT );
        p.setColor( QPalette::HighlightedText, TomahawkStyle::PAGE_TRACKLIST_HIGHLIGHT_TEXT );

        ui->tracksView->setPalette( p );
        ui->tracksView->setFrameShape( QFrame::NoFrame );
        ui->tracksView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        ui->tracksView->setStyleSheet( "QTreeView { background-color: transparent; }" );
        TomahawkStyle::stylePageFrame( ui->trackFrame );
    }

    {
        RecentPlaylistsModel* model = new RecentPlaylistsModel( HISTORY_PLAYLIST_ITEMS, this );

        ui->playlistWidget->setFrameShape( QFrame::NoFrame );
        ui->playlistWidget->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        ui->playlistWidget->setItemDelegate( new PlaylistDelegate() );
        ui->playlistWidget->setModel( model );
        ui->playlistWidget->overlay()->resize( 380, 86 );
        ui->playlistWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

        QPalette p = ui->playlistWidget->palette();
        p.setColor( QPalette::Text, TomahawkStyle::HEADER_TEXT );
        p.setColor( QPalette::BrightText, TomahawkStyle::HEADER_TEXT );
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );
        p.setColor( QPalette::Highlight, TomahawkStyle::HEADER_TEXT );
        p.setColor( QPalette::HighlightedText, TomahawkStyle::HEADER_BACKGROUND );

        ui->playlistWidget->setPalette( p );
        ui->playlistWidget->setMinimumHeight( 400 );
        ui->playlistWidget->setStyleSheet( "QListView { background-color: transparent; }" );
        TomahawkStyle::styleScrollBar( ui->playlistWidget->verticalScrollBar() );
        TomahawkStyle::stylePageFrame( ui->playlistFrame );

        updatePlaylists();
        connect( ui->playlistWidget, SIGNAL( activated( QModelIndex ) ), SLOT( onPlaylistActivated( QModelIndex ) ) );
        connect( model, SIGNAL( emptinessChanged( bool ) ), this, SLOT( updatePlaylists() ) );
    }

    {
        m_recentAlbumsModel = new AlbumModel( ui->additionsView );
        ui->additionsView->setPlayableModel( m_recentAlbumsModel );
        ui->additionsView->proxyModel()->sort( -1 );

        ui->additionsView->setStyleSheet( "QListView { background-color: transparent; }" );
        TomahawkStyle::stylePageFrame( ui->additionsFrame );
        TomahawkStyle::styleScrollBar( ui->additionsView->verticalScrollBar() );
    }

    {
        QFont f = ui->label->font();
        f.setFamily( "Pathway Gothic One" );

        QPalette p = ui->label->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_CAPTION );

        ui->label->setFont( f );
        ui->label_2->setFont( f );
        ui->label->setPalette( p );
        ui->label_2->setPalette( p );
    }

    {
        QFont f = ui->playlistLabel->font();
        f.setFamily( "Pathway Gothic One" );

        QPalette p = ui->playlistLabel->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );

        ui->playlistLabel->setFont( f );
        ui->playlistLabel->setPalette( p );
    }

    {
        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        area->setWidget( widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND );
        area->setPalette( pal );
        area->setAutoFillBackground( true );
        area->setFrameShape( QFrame::NoFrame );
        area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( area );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    {
        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        ui->widget->setPalette( pal );
        ui->widget->setAutoFillBackground( true );
    }

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->tracksView->playlistInterface() );
    mpl->addChildInterface( ui->additionsView->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( mpl );

    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( onSourcesReady() ) );
    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
}


Dashboard::~Dashboard()
{
    delete ui;
}


Tomahawk::playlistinterface_ptr
Dashboard::playlistInterface() const
{
    return m_playlistInterface;
}


bool
Dashboard::jumpToCurrentTrack()
{
    if ( ui->tracksView->jumpToCurrentTrack() )
        return true;

    if ( ui->additionsView->jumpToCurrentTrack() )
        return true;

    return false;
}


bool
Dashboard::isBeingPlayed() const
{
    if ( ui->additionsView->isBeingPlayed() )
        return true;

    return AudioEngine::instance()->currentTrackPlaylist() == ui->tracksView->playlistInterface();
}


void
Dashboard::onSourcesReady()
{
    foreach ( const source_ptr& source, SourceList::instance()->sources() )
        onSourceAdded( source );

    updateRecentAdditions();
}


void
Dashboard::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->dbCollection().data(), SIGNAL( changed() ), SLOT( updateRecentAdditions() ), Qt::UniqueConnection );
}


void
Dashboard::updateRecentAdditions()
{
    m_recentAlbumsModel->addFilteredCollection( collection_ptr(), 20, DatabaseCommand_AllAlbums::ModificationTime, true );
}


void
Dashboard::updatePlaylists()
{
    int num = ui->playlistWidget->model()->rowCount( QModelIndex() );
    if ( num == 0 )
    {
        ui->playlistWidget->overlay()->setText( tr( "No recently created playlists in your network." ) );
        ui->playlistWidget->overlay()->show();
    }
    else
        ui->playlistWidget->overlay()->hide();
}


void
Dashboard::onPlaylistActivated( const QModelIndex& item )
{
    Tomahawk::playlist_ptr pl = item.data( RecentlyPlayedPlaylistsModel::PlaylistRole ).value< Tomahawk::playlist_ptr >();
    if ( Tomahawk::dynplaylist_ptr dynplaylist = pl.dynamicCast< Tomahawk::DynamicPlaylist >() )
        ViewManager::instance()->show( dynplaylist );
    else
        ViewManager::instance()->show( pl );
}


void
Dashboard::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


QPixmap
Dashboard::pixmap() const
{
    return ImageRegistry::instance()->pixmap( RESPATH "images/dashboard.svg", QSize( 0, 0 ) );
}


QSize
PlaylistDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    // Calculates the size for the bold line + 3 normal lines + margins
    int height = 2 * 6; // margins
    QFont font = option.font;
    QFontMetrics fm1( font );
    font.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    height += fm1.height() * 3;
    font.setPointSize( TomahawkUtils::defaultFontSize() );
    QFontMetrics fm2( font );
    height += fm2.height();

    return QSize( 0, height );
}


void
PlaylistDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( option.state & QStyle::State_Selected && option.state & QStyle::State_Active )
    {
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
    }

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

    QTextOption to;
    to.setAlignment( Qt::AlignCenter );
    QFont font = opt.font;
    font.setPointSize( TomahawkUtils::defaultFontSize() - 1 );

    QFont boldFont = font;
    boldFont.setBold( true );
    boldFont.setPointSize( TomahawkUtils::defaultFontSize() );
    QFontMetrics boldFontMetrics( boldFont );

    QFont figFont = boldFont;
    figFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );

    QPixmap icon;
    QRect pixmapRect = option.rect.adjusted( 10, 14, -option.rect.width() + option.rect.height() - 18, -14 );
    RecentlyPlayedPlaylistsModel::PlaylistTypes type = (RecentlyPlayedPlaylistsModel::PlaylistTypes)index.data( RecentlyPlayedPlaylistsModel::PlaylistTypeRole ).toInt();

    if ( type == RecentlyPlayedPlaylistsModel::StaticPlaylist )
        icon = TomahawkUtils::defaultPixmap( TomahawkUtils::Playlist, TomahawkUtils::Original, pixmapRect.size() );
    else if ( type == RecentlyPlayedPlaylistsModel::AutoPlaylist )
        icon = TomahawkUtils::defaultPixmap( TomahawkUtils::AutomaticPlaylist, TomahawkUtils::Original, pixmapRect.size() );
    else if ( type == RecentlyPlayedPlaylistsModel::Station )
        icon = TomahawkUtils::defaultPixmap( TomahawkUtils::Station, TomahawkUtils::Original, pixmapRect.size() );

    painter->drawPixmap( pixmapRect, icon );

    if ( type != RecentlyPlayedPlaylistsModel::Station )
    {
        painter->save();
        painter->setFont( figFont );
        QString tracks = index.data( RecentlyPlayedPlaylistsModel::TrackCountRole ).toString();
        int width = painter->fontMetrics().width( tracks );
//         int bottomEdge = pixmapRect
        // right edge 10px past right edge of pixmapRect
        // bottom edge flush with bottom of pixmap
        QRect rect( pixmapRect.right() - width, 0, width - 8, 0 );
        rect.adjust( -2, 0, 0, 0 );
        rect.setTop( pixmapRect.bottom() - painter->fontMetrics().height() - 1 );
        rect.setBottom( pixmapRect.bottom() + 1 );

        QColor figColor( TomahawkStyle::DASHBOARD_ROUNDFIGURE_BACKGROUND );
        painter->setPen( Qt::white );
        painter->setBrush( figColor );

        TomahawkUtils::drawBackgroundAndNumbers( painter, tracks, rect );
        painter->restore();
    }

    QRect r( option.rect.width() - option.fontMetrics.height() * 2.5 - 10, option.rect.top() + option.rect.height() / 3 - option.fontMetrics.height(), option.fontMetrics.height() * 2.5, option.fontMetrics.height() * 2.5 );
    QPixmap avatar = index.data( RecentlyPlayedPlaylistsModel::PlaylistRole ).value< Tomahawk::playlist_ptr >()->author()->avatar( TomahawkUtils::RoundedCorners, r.size() );
    if ( avatar.isNull() )
        avatar = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultSourceAvatar, TomahawkUtils::RoundedCorners, r.size() );
    painter->drawPixmap( r, avatar );

    painter->setFont( font );
    QString author = index.data( RecentlyPlayedPlaylistsModel::PlaylistRole ).value< Tomahawk::playlist_ptr >()->author()->friendlyName();
    if ( author.indexOf( '@' ) > 0 )
        author = author.mid( 0, author.indexOf( '@' ) );

    const int w = painter->fontMetrics().width( author ) + 2;
    QRect avatarNameRect( opt.rect.width() - 10 - w, r.bottom(), w, opt.rect.bottom() - r.bottom() );
    painter->drawText( avatarNameRect, author, QTextOption( Qt::AlignCenter ) );

    const int leftEdge = opt.rect.width() - qMin( avatarNameRect.left(), r.left() );
    QString descText;
    if ( type == RecentlyPlayedPlaylistsModel::Station )
    {
        descText = index.data( RecentlyPlayedPlaylistsModel::DynamicPlaylistRole ).value< Tomahawk::dynplaylist_ptr >()->generator()->sentenceSummary();
    }
    else
    {
        descText = index.data( RecentlyPlayedPlaylistsModel::ArtistRole ).toString();
    }

    QColor c = painter->pen().color();
    if ( !( option.state & QStyle::State_Selected && option.state & QStyle::State_Active ) )
    {
        painter->setPen( opt.palette.text().color().darker() );
    }

    QRect rectText = option.rect.adjusted( option.fontMetrics.height() * 4.5, boldFontMetrics.height() + 6, -leftEdge - 10, -8 );
#ifdef Q_WS_MAC
    rectText.adjust( 0, 1, 0, 0 );
#elif defined Q_WS_WIN
    rectText.adjust( 0, 2, 0, 0 );
#endif

    painter->drawText( rectText, descText );
    painter->setPen( c );
    painter->setFont( font );

    painter->setFont( boldFont );
    painter->drawText( option.rect.adjusted( option.fontMetrics.height() * 4, 6, -100, -option.rect.height() + boldFontMetrics.height() + 6 ), index.data().toString() );

    painter->restore();
}
