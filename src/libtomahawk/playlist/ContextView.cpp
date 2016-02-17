/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ContextView.h"

#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "DownloadManager.h"
#include "Result.h"
#include "audio/AudioEngine.h"
#include "widgets/CaptionLabel.h"
#include "playlist/PlayableModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TrackView.h"
#include "playlist/GridView.h"
#include "playlist/TrackItemDelegate.h"
#include "playlist/TrackDetailView.h"
#include "PlayableProxyModelPlaylistInterface.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/ImageRegistry.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

using namespace Tomahawk;


ContextView::ContextView( QWidget* parent, const QString& caption )
    : QWidget( parent )
    , m_trackView( new TrackView() )
    , m_temporary( false )
{
    TrackItemDelegate* del = new TrackItemDelegate( TrackItemDelegate::LovedTracks, m_trackView, m_trackView->proxyModel() );
    m_trackView->setPlaylistItemDelegate( del );
    m_trackView->proxyModel()->setStyle( PlayableProxyModel::SingleColumn );
    m_trackView->setStyleSheet( QString( "QTreeView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );
#ifndef Q_OS_MAC
    TomahawkStyle::styleScrollBar( m_trackView->verticalScrollBar() );
#endif

    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    m_captionLabel = new CaptionLabel( this );
    setCaption( caption );

    QWidget* vbox = new QWidget;
    QPalette pal = vbox->palette();
    pal.setBrush( vbox->backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
    vbox->setPalette( pal );
    vbox->setAutoFillBackground( true );

    QVBoxLayout* vboxl = new QVBoxLayout();
    TomahawkUtils::unmarginLayout( vboxl );
    vboxl->setContentsMargins( 32, 32, 32, 32 );
    vboxl->setSpacing( 32 );

    vbox->setLayout( vboxl );

    QWidget* hbox = new QWidget;
    QHBoxLayout* hboxl = new QHBoxLayout();
    TomahawkUtils::unmarginLayout( hboxl );
    hboxl->setSpacing( 32 );

    m_innerLayout = new QVBoxLayout();
    TomahawkUtils::unmarginLayout( m_innerLayout );
    m_innerLayout->addWidget( m_trackView, 1 );
    m_innerLayout->addStretch();

    m_detailView = new TrackDetailView;
    m_detailView->setPlaylistInterface( playlistInterface() );
    hboxl->addWidget( m_detailView );
    hboxl->addLayout( m_innerLayout );
    hbox->setLayout( hboxl );

    vboxl->addWidget( m_captionLabel );
    vboxl->addWidget( hbox );
    layout()->addWidget( vbox );

    connect( m_captionLabel, SIGNAL( clicked() ), SIGNAL( closeClicked() ) );
    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), SLOT( onQuerySelected( Tomahawk::query_ptr ) ) );
    connect( m_trackView, SIGNAL( modelChanged() ), SLOT( onModelChanged() ) );
    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), m_detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );
    connect( m_detailView, SIGNAL( downloadAll() ), SLOT( onDownloadAll() ) );
    connect( m_detailView, SIGNAL( downloadCancel() ), SLOT( onDownloadCancel() ) );

    TomahawkUtils::fixMargins( this );
}


ContextView::~ContextView()
{
    tDebug() << Q_FUNC_INFO;
}


void
ContextView::setTrackView( TrackView* view )
{
    if ( m_trackView )
    {
        disconnect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), this, SLOT( onQuerySelected( Tomahawk::query_ptr ) ) );
        disconnect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), m_detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );
        disconnect( m_trackView, SIGNAL( modelChanged() ), this, SLOT( onModelChanged() ) );

        m_innerLayout->removeWidget( m_trackView );
        delete m_trackView;
    }

    m_trackView = view;
    m_trackView->setStyleSheet( QString( "QTreeView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );
#ifndef Q_OS_MAC
    TomahawkStyle::styleScrollBar( m_trackView->verticalScrollBar() );
#endif

    m_innerLayout->insertWidget( 0, view, 1 );

    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), SLOT( onQuerySelected( Tomahawk::query_ptr ) ) );
    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), m_detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );
    connect( m_trackView, SIGNAL( modelChanged() ), SLOT( onModelChanged() ) );
}


void
ContextView::setCaption( const QString& caption )
{
    if ( caption.isEmpty() )
        m_captionLabel->setText( tr( "Playlist Details" ) );
    else
        m_captionLabel->setText( caption );
}


void
ContextView::onDownloadAll()
{
    for ( int i = 0; i < m_trackView->proxyModel()->rowCount( QModelIndex() ); i++ )
    {
        PlayableItem* item = m_trackView->proxyModel()->itemFromIndex( m_trackView->proxyModel()->mapToSource( m_trackView->proxyModel()->index( i, 0, QModelIndex() ) ) );
        if ( !item || !item->query() || !item->query()->results().count() )
            continue;
        if ( !item->query()->results().first()->downloadFormats().count() )
            continue;

        if ( !DownloadManager::instance()->localFileForDownload( item->query()->results().first()->downloadFormats().first().url.toString() ).isEmpty() )
            continue;
        if ( !item->result()->downloadFormats().isEmpty() )
            DownloadManager::instance()->addJob( item->result()->toDownloadJob( item->result()->downloadFormats().first() ) );
    }
}


void
ContextView::onDownloadCancel()
{
    DownloadManager::instance()->cancelAll();
}


void
ContextView::onQuerySelected( const Tomahawk::query_ptr& query )
{
    if ( m_query )
    {
        disconnect( m_query->track().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
    }

    m_query = query;
    m_detailView->setQuery( m_query );

    if ( m_query )
    {
        connect( m_query->track().data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );
    }

    onCoverUpdated();
}


void
ContextView::onCoverUpdated()
{
    if ( !m_query || m_query->track()->cover( QSize( 0, 0 ) ).isNull() )
    {
        m_pixmap = QPixmap();
    }
    else
    {
        m_pixmap = m_query->track()->cover( QSize( 0, 0 ) );
    }

    emit pixmapChanged( m_pixmap );
}


void
ContextView::setGuid( const QString& guid )
{
    m_trackView->setGuid( guid );
}


Tomahawk::playlistinterface_ptr
ContextView::playlistInterface() const
{
    return m_trackView->proxyModel()->playlistInterface();
}


QString
ContextView::title() const
{
    return m_trackView->title();
}


QString
ContextView::description() const
{
    return m_trackView->description();
}


QPixmap
ContextView::pixmap() const
{
    return m_pixmap;
}


bool
ContextView::jumpToCurrentTrack()
{
    tDebug() << Q_FUNC_INFO;

    return m_trackView->jumpToCurrentTrack();
}


bool
ContextView::setFilter( const QString& pattern )
{
    ViewPage::setFilter( pattern );

    m_trackView->setFilter( pattern );

    return true;
}


void
ContextView::setEmptyTip( const QString& tip )
{
    m_trackView->setEmptyTip( tip );
}


void
ContextView::onModelChanged()
{
    if ( m_trackView->model()->isReadOnly() )
        setEmptyTip( tr( "This playlist is currently empty." ) );
    else
        setEmptyTip( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );

    emit modelChanged();
}


void
ContextView::onWidgetDestroyed( QWidget* widget )
{
    Q_UNUSED( widget );
    emit destroyed( this );
}


bool
ContextView::isTemporaryPage() const
{
    return m_temporary;
}


void
ContextView::setTemporaryPage( bool b )
{
    m_temporary = b;
}


bool
ContextView::isBeingPlayed() const
{
    if ( !playlistInterface() )
        return false;

    if ( playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}


void
ContextView::setShowCloseButton( bool b )
{
    m_captionLabel->setShowCloseButton( b );
}


TrackDetailView*
ContextView::trackDetailView() const
{
    return m_detailView;
}
