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

#include "audio/AudioEngine.h"
#include "widgets/CaptionLabel.h"
#include "widgets/FilterHeader.h"
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
//    , m_header( new FilterHeader( this ) )
    , m_trackView( new TrackView() )
    , m_model( 0 )
    , m_temporary( false )
{
//    qRegisterMetaType< ContextViewMode >( "ContextViewMode" );

/*    m_detailedView->setColumnHidden( PlayableModel::Age, true ); // Hide age column per default
    m_detailedView->setColumnHidden( PlayableModel::Filesize, true ); // Hide filesize column per default
    m_detailedView->setColumnHidden( PlayableModel::Composer, true ); // Hide composer column per default*/

    TrackItemDelegate* del = new TrackItemDelegate( TrackItemDelegate::LovedTracks, m_trackView, m_trackView->proxyModel() );
    m_trackView->setPlaylistItemDelegate( del );
    m_trackView->proxyModel()->setStyle( PlayableProxyModel::Large );
    TomahawkStyle::styleScrollBar( m_trackView->verticalScrollBar() );

    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    m_trackView->setStyleSheet( QString( "QTreeView { background-color: white; }" ) );
//    m_gridView->setStyleSheet( QString( "QListView { background-color: white; }" ) );

    m_captionLabel = new CaptionLabel( this );
    setCaption( caption );

    QWidget* vbox = new QWidget;
    QPalette pal = vbox->palette();
    pal.setBrush( vbox->backgroundRole(), Qt::white );
    vbox->setPalette( pal );
    vbox->setAutoFillBackground( true );

    QVBoxLayout* vboxl = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( vboxl );
    vboxl->setContentsMargins( 32, 32, 32, 32 );
    vboxl->setSpacing( 32 );

    vbox->setLayout( vboxl );

    QWidget* hbox = new QWidget;
    QHBoxLayout* hboxl = new QHBoxLayout;
    TomahawkUtils::unmarginLayout( hboxl );
    hboxl->setSpacing( 32 );

    QVBoxLayout* vboxInner = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( vboxInner );
    vboxInner->addWidget( m_trackView );
    vboxInner->addStretch();
    vboxInner->setStretchFactor( m_trackView, 1 );

    TrackDetailView* detailView = new TrackDetailView;
    detailView->setPlaylistInterface( playlistInterface() );
    hboxl->addWidget( detailView );
    hboxl->addLayout( vboxInner );
    hbox->setLayout( hboxl );

    vboxl->addWidget( m_captionLabel );
    vboxl->addWidget( hbox );
    layout()->addWidget( vbox );

    connect( m_captionLabel, SIGNAL( clicked() ), SIGNAL( closeClicked() ) );
    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), SLOT( onQuerySelected( Tomahawk::query_ptr ) ) );
    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );
//    connect( m_header, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );
}


ContextView::~ContextView()
{
    tDebug() << Q_FUNC_INFO;
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
ContextView::onQuerySelected( const Tomahawk::query_ptr& query )
{
    if ( m_query )
    {
        disconnect( m_query->track().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
    }

    m_query = query;

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
        emit pixmapChanged( m_pixmap );
        return;
    }

    m_pixmap = m_query->track()->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );
}


void
ContextView::setGuid( const QString& guid )
{
    m_trackView->setGuid( guid );
}


void
ContextView::setPlayableModel( PlayableModel* model )
{
    if ( m_model )
    {
        delete m_model;
    }

    m_model = model;

    m_trackView->setPlayableModel( model );
    m_trackView->setSortingEnabled( false );
    m_trackView->sortByColumn( -1 );
    m_trackView->proxyModel()->sort( -1 );

    onModelChanged();
}


void
ContextView::setPlaylistModel( PlaylistModel* model )
{
    if ( m_model )
    {
        disconnect( m_model, SIGNAL( changed() ), this, SLOT( onModelChanged() ) );
    }

    setPlayableModel( model );

    connect( model, SIGNAL( changed() ), SLOT( onModelChanged() ), Qt::UniqueConnection );
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
ContextView::setPixmap( const QPixmap& pixmap )
{
    m_pixmap = pixmap;
//    m_header->setPixmap( pixmap );
}


void
ContextView::onModelChanged()
{
//    m_header->setPixmap( m_pixmap );
//    m_header->setCaption( m_model->title() );
//    m_header->setDescription( m_model->description() );

    if ( m_model->isReadOnly() )
        setEmptyTip( tr( "This playlist is currently empty." ) );
    else
        setEmptyTip( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );
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
