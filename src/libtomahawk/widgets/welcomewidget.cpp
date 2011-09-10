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

#include "welcomewidget.h"
#include "ui_welcomewidget.h"

#include <QPainter>

#include "viewmanager.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
#include "RecentPlaylistsModel.h"

#include "audio/audioengine.h"
#include "playlist/albummodel.h"
#include "playlist/playlistmodel.h"
#include "widgets/overlaywidget.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include <dynamic/GeneratorInterface.h>
#include "RecentlyPlayedPlaylistsModel.h"

#define HISTORY_TRACK_ITEMS 25
#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_RESOLVING_TIMEOUT 2500

using namespace Tomahawk;


WelcomeWidget::WelcomeWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WelcomeWidget )
{
    ui->setupUi( this );

    ui->splitter_2->setStretchFactor( 0, 2 );
    ui->splitter_2->setStretchFactor( 0, 1 );

    RecentPlaylistsModel* model = new RecentPlaylistsModel( HISTORY_PLAYLIST_ITEMS, this );

    ui->playlistWidget->setFrameShape( QFrame::NoFrame );
    ui->playlistWidget->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->tracksView->setFrameShape( QFrame::NoFrame );
    ui->tracksView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->additionsView->setFrameShape( QFrame::NoFrame );
    ui->additionsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_3->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_4->layout() );

    ui->playlistWidget->setItemDelegate( new PlaylistDelegate() );
    ui->playlistWidget->setModel( model );
    ui->playlistWidget->overlay()->resize( 380, 86 );
    ui->playlistWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    connect( model, SIGNAL( emptinessChanged( bool) ), this, SLOT( updatePlaylists() ) );

    m_tracksModel = new PlaylistModel( ui->tracksView );
    m_tracksModel->setStyle( TrackModel::ShortWithAvatars );
    ui->tracksView->overlay()->setEnabled( false );
    ui->tracksView->setPlaylistModel( m_tracksModel );

    m_recentAlbumsModel = new AlbumModel( ui->additionsView );
    ui->additionsView->setAlbumModel( m_recentAlbumsModel );
    m_recentAlbumsModel->addFilteredCollection( collection_ptr(), 20, DatabaseCommand_AllAlbums::ModificationTime, true );

    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), SLOT( checkQueries() ) );

    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( updateRecentTracks() ) );
    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    connect( ui->playlistWidget, SIGNAL( activated( QModelIndex ) ), SLOT( onPlaylistActivated( QModelIndex ) ) );
    connect( AudioEngine::instance() ,SIGNAL( playlistChanged( Tomahawk::PlaylistInterface* ) ), SLOT( updatePlaylists() ), Qt::QueuedConnection );
}


WelcomeWidget::~WelcomeWidget()
{
    delete ui;
}


void
WelcomeWidget::updateRecentTracks()
{
    m_tracksModel->loadHistory( Tomahawk::source_ptr(), HISTORY_TRACK_ITEMS );

    connect( SourceList::instance()->getLocal().data(), SIGNAL( stats( QVariantMap ) ), this, SLOT( updateRecentAdditions() ) );
}


void
WelcomeWidget::updateRecentAdditions()
{
    m_recentAlbumsModel->addFilteredCollection( collection_ptr(), 20, DatabaseCommand_AllAlbums::ModificationTime, true );
}


void
WelcomeWidget::updatePlaylists()
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
WelcomeWidget::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ) );
}


void
WelcomeWidget::checkQueries()
{
    if ( m_timer->isActive() )
        m_timer->stop();

    m_tracksModel->ensureResolved();
}


void
WelcomeWidget::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    m_tracksModel->insert( 0, query );

    if ( m_tracksModel->trackCount() > HISTORY_TRACK_ITEMS )
        m_tracksModel->remove( HISTORY_TRACK_ITEMS );

    if ( m_timer->isActive() )
        m_timer->stop();
    m_timer->start( HISTORY_RESOLVING_TIMEOUT );
}


void
WelcomeWidget::onPlaylistActivated( const QModelIndex& item )
{
    qDebug() << Q_FUNC_INFO;

    Tomahawk::playlist_ptr pl = item.data( RecentlyPlayedPlaylistsModel::PlaylistRole ).value< Tomahawk::playlist_ptr >();
    if( Tomahawk::dynplaylist_ptr dynplaylist = pl.dynamicCast< Tomahawk::DynamicPlaylist >() )
        ViewManager::instance()->show( dynplaylist );
    else
        ViewManager::instance()->show( pl );
}


void
WelcomeWidget::changeEvent( QEvent* e )
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


QSize
PlaylistDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );
    return QSize( 0, 64 );
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
#ifdef Q_WS_MAC
    font.setPointSize( font.pointSize() - 2 );
#endif

    QFont boldFont = font;
    boldFont.setBold( true );

    QPixmap icon;
    RecentlyPlayedPlaylistsModel::PlaylistTypes type = (RecentlyPlayedPlaylistsModel::PlaylistTypes)index.data( RecentlyPlayedPlaylistsModel::PlaylistTypeRole ).toInt();
    if( type == RecentlyPlayedPlaylistsModel::StaticPlaylist )
        icon = m_playlistIcon;
    else if( type == RecentlyPlayedPlaylistsModel::AutoPlaylist )
        icon = m_autoIcon;
    else if( type == RecentlyPlayedPlaylistsModel::Station )
        icon = m_stationIcon;

    QRect pixmapRect = option.rect.adjusted( 10, 13, -option.rect.width() + 48, -13 );
    icon = icon.scaled( pixmapRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

    painter->drawPixmap( pixmapRect, icon );

    QString descText;
    if ( type == RecentlyPlayedPlaylistsModel::Station )
    {
        descText = index.data( RecentlyPlayedPlaylistsModel::DynamicPlaylistRole ).value< Tomahawk::dynplaylist_ptr >()->generator()->sentenceSummary();
    } else
    {
        descText = index.data( RecentlyPlayedPlaylistsModel::ArtistRole ).toString();
    }
    QColor c = painter->pen().color();
    painter->setPen( QColor( Qt::gray ).darker() );
    QFont font2 = font;
    font2.setPointSize( font2.pointSize() - 1 );
    painter->setFont( font2 );

    QRect rectText = option.rect.adjusted( 66, 20, -100, -8 );
#ifdef Q_WS_MAC
    rectText.adjust( 0, 1, 0, 0 );
#elif defined Q_WS_WIN
    rectText.adjust( 0, 2, 0, 0 );
#endif

    painter->drawText( rectText, descText );
    painter->setPen( c );
    painter->setFont( font );

    if ( type != RecentlyPlayedPlaylistsModel::Station )
    {
        painter->save();
        QString tracks = index.data( RecentlyPlayedPlaylistsModel::TrackCountRole ).toString();
        int width = painter->fontMetrics().width( tracks );
//         int bottomEdge = pixmapRect
        // right edge 10px past right edge of pixmapRect
        // bottom edge flush with bottom of pixmap
        QRect rect( pixmapRect.right() - width , 0, width - 8, 0 );
        rect.adjust( -1, 0, 0, 0 );
        rect.setTop( pixmapRect.bottom() - painter->fontMetrics().height() - 1 );
        rect.setBottom( pixmapRect.bottom() + 1 );

        QColor figColor( 153, 153, 153 );
        painter->setPen( figColor );
        painter->setBrush( figColor );
        painter->setFont( boldFont );

        TomahawkUtils::drawBackgroundAndNumbers( painter, tracks, rect );
        painter->restore();
    }

    QPixmap avatar = index.data( RecentlyPlayedPlaylistsModel::PlaylistRole ).value< Tomahawk::playlist_ptr >()->author()->avatar( Source::FancyStyle );
    if ( avatar.isNull() )
        avatar = m_defaultAvatar;
    QRect r( option.rect.width() - avatar.width() - 10, option.rect.top() + option.rect.height()/2 - avatar.height()/2, avatar.width(), avatar.height() );
    painter->drawPixmap( r, avatar );

    painter->setFont( boldFont );
    painter->drawText( option.rect.adjusted( 56, 6, -100, -option.rect.height() + 20 ), index.data().toString() );

    painter->restore();
}


PlaylistWidget::PlaylistWidget( QWidget* parent )
    : QListView( parent )
{
    m_overlay = new OverlayWidget( this );
}
