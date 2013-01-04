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

#include "WelcomeWidget.h"
#include "ui_WelcomeWidget.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "RecentPlaylistsModel.h"
#include "MetaPlaylistInterface.h"

#include "audio/AudioEngine.h"
#include "playlist/AlbumModel.h"
#include "playlist/RecentlyPlayedModel.h"
#include "widgets/OverlayWidget.h"
#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "RecentlyPlayedPlaylistsModel.h"

#include <QPainter>


#define HISTORY_PLAYLIST_ITEMS 10

using namespace Tomahawk;


WelcomeWidget::WelcomeWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WelcomeWidget )
{
    ui->setupUi( this );

    ui->splitter_2->setStretchFactor( 0, 3 );
    ui->splitter_2->setStretchFactor( 1, 1 );
    ui->splitter->setChildrenCollapsible( false );
    ui->splitter_2->setChildrenCollapsible( false );

    RecentPlaylistsModel* model = new RecentPlaylistsModel( HISTORY_PLAYLIST_ITEMS, this );

    ui->playlistWidget->setFrameShape( QFrame::NoFrame );
    ui->playlistWidget->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_3->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_4->layout() );

    ui->playlistWidget->setItemDelegate( new PlaylistDelegate() );
    ui->playlistWidget->setModel( model );
    ui->playlistWidget->overlay()->resize( 380, 86 );
    ui->playlistWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    updatePlaylists();

    m_tracksModel = new RecentlyPlayedModel( ui->tracksView );
    ui->tracksView->proxyModel()->setStyle( PlayableProxyModel::ShortWithAvatars );
    ui->tracksView->overlay()->setEnabled( false );
    ui->tracksView->setPlaylistModel( m_tracksModel );
    m_tracksModel->setSource( source_ptr() );

    QFont f;
    f.setBold( true );
    QFontMetrics fm( f );
    ui->tracksView->setMinimumWidth( fm.width( tr( "Recently played tracks" ) ) * 2 );

    m_recentAlbumsModel = new AlbumModel( ui->additionsView );
    ui->additionsView->setPlayableModel( m_recentAlbumsModel );
    ui->additionsView->proxyModel()->sort( -1 );

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->tracksView->playlistInterface() );
    mpl->addChildInterface( ui->additionsView->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( mpl );

    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( onSourcesReady() ) );
    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    connect( ui->playlistWidget, SIGNAL( activated( QModelIndex ) ), SLOT( onPlaylistActivated( QModelIndex ) ) );
    connect( model, SIGNAL( emptinessChanged( bool ) ), this, SLOT( updatePlaylists() ) );
}


WelcomeWidget::~WelcomeWidget()
{
    delete ui;
}


void
WelcomeWidget::loadData()
{
    m_recentAlbumsModel->addFilteredCollection( collection_ptr(), 20, DatabaseCommand_AllAlbums::ModificationTime, true );
}


Tomahawk::playlistinterface_ptr
WelcomeWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
WelcomeWidget::jumpToCurrentTrack()
{
    if ( ui->tracksView->jumpToCurrentTrack() )
        return true;

    if ( ui->additionsView->jumpToCurrentTrack() )
        return true;

    return false;
}


bool
WelcomeWidget::isBeingPlayed() const
{
    if ( ui->additionsView->isBeingPlayed() )
        return true;

    return AudioEngine::instance()->currentTrackPlaylist() == ui->tracksView->playlistInterface();
}


void
WelcomeWidget::onSourcesReady()
{
    foreach ( const source_ptr& source, SourceList::instance()->sources() )
        onSourceAdded( source );
}


void
WelcomeWidget::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->collection().data(), SIGNAL( changed() ), SLOT( updateRecentAdditions() ), Qt::UniqueConnection );
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
WelcomeWidget::onPlaylistActivated( const QModelIndex& item )
{
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

        QColor figColor( "#454e59" );
        painter->setPen( figColor );
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
        painter->setPen( QColor( Qt::gray ).darker() );
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


PlaylistWidget::PlaylistWidget( QWidget* parent )
    : QListView( parent )
{
    m_overlay = new OverlayWidget( this );
    /* LoadingSpinner* spinner = */ new LoadingSpinner( this );
}


void
PlaylistWidget::setModel( QAbstractItemModel* model )
{
    QListView::setModel( model );
    emit modelChanged();
}

#include "WelcomeWidget.moc"
