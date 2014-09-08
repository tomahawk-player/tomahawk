/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "FlexibleView.h"

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


FlexibleView::FlexibleView( QWidget* parent, QWidget* extraHeader )
    : QWidget( parent )
    , m_header( new FilterHeader( this ) )
    , m_trackView( new TrackView() )
    , m_detailedView( new TrackView() )
    , m_gridView( new GridView() )
    , m_model( 0 )
    , m_temporary( false )
{
    qRegisterMetaType< FlexibleViewMode >( "FlexibleViewMode" );

    m_header->setBackground( ImageRegistry::instance()->pixmap( RESPATH "images/playlist_background.png", QSize( 0, 0 ) ) );

//    m_trackView->setPlaylistInterface( m_playlistInterface );
    m_detailedView->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );
    m_gridView->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );

    m_detailedView->setColumnHidden( PlayableModel::Age, true ); // Hide age column per default
    m_detailedView->setColumnHidden( PlayableModel::Filesize, true ); // Hide filesize column per default
    m_detailedView->setColumnHidden( PlayableModel::Composer, true ); // Hide composer column per default

    TrackItemDelegate* del = new TrackItemDelegate( TrackItemDelegate::LovedTracks, m_trackView, m_trackView->proxyModel() );
    m_trackView->setPlaylistItemDelegate( del );
    m_trackView->proxyModel()->setStyle( PlayableProxyModel::Large );

    m_stack = new QStackedWidget();
    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    m_trackView->setStyleSheet( QString( "QTreeView { background-color: white; }" ) );
    m_gridView->setStyleSheet( QString( "QListView { background-color: white; }" ) );
    TomahawkStyle::styleScrollBar( m_trackView->verticalScrollBar() );

    layout()->addWidget( m_header );
    if ( extraHeader )
        layout()->addWidget( extraHeader );

    CaptionLabel* caption = new CaptionLabel( this );
    caption->setText( tr( "Playlist Details" ) );

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

    m_detailView = new TrackDetailView;
    m_detailView->setPlaylistInterface( playlistInterface() );
    hboxl->addWidget( m_detailView );
    hboxl->addWidget( m_stack );
    hbox->setLayout( hboxl );

    vboxl->addWidget( caption );
    vboxl->addWidget( hbox );
    layout()->addWidget( vbox );

    m_stack->addWidget( m_trackView );
    m_stack->addWidget( m_detailedView );
    m_stack->addWidget( m_gridView );

    setCurrentMode( Flat );

    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), m_detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );
    connect( m_header, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );

    TomahawkUtils::fixMargins( this );
}


FlexibleView::~FlexibleView()
{
    tDebug() << Q_FUNC_INFO;
}


void
FlexibleView::setGuid( const QString& guid )
{
    m_trackView->setGuid( guid );
    m_detailedView->setGuid( guid );
}


void
FlexibleView::setTrackView( TrackView* view )
{
    if ( m_trackView )
    {
        disconnect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), m_detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );

        m_stack->removeWidget( m_trackView );
        delete m_trackView;
    }

    m_trackView = view;
    m_stack->addWidget( view );

    //    view->setPlaylistInterface( m_playlistInterface );
    m_detailView->setPlaylistInterface( playlistInterface() );

    m_trackView->setStyleSheet( QString( "QTreeView { background-color: white; }" ) );
    TomahawkStyle::styleScrollBar( m_trackView->verticalScrollBar() );
    connect( m_trackView, SIGNAL( querySelected( Tomahawk::query_ptr ) ), m_detailView, SLOT( setQuery( Tomahawk::query_ptr ) ) );
}


void
FlexibleView::setDetailedView( TrackView* view )
{
    if ( m_detailedView )
    {
        m_stack->removeWidget( m_detailedView );
        delete m_detailedView;
    }

    connect( view, SIGNAL( destroyed( QWidget* ) ), SLOT( onWidgetDestroyed( QWidget* ) ), Qt::UniqueConnection );

    view->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );

    m_detailedView = view;
    m_stack->addWidget( view );
}


void
FlexibleView::setGridView( GridView* view )
{
    if ( m_gridView )
    {
        m_stack->removeWidget( m_gridView );
        delete m_gridView;
    }

    view->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );

    m_gridView = view;
    m_stack->addWidget( view );
}


void
FlexibleView::setPlayableModel( PlayableModel* model )
{
    if ( m_model )
    {
        delete m_model;
    }

    m_model = model;

    m_trackView->setPlayableModel( model );
    m_detailedView->setPlayableModel( model );
    m_gridView->setPlayableModel( model );

    m_trackView->setSortingEnabled( false );
    m_trackView->sortByColumn( -1 );
    m_trackView->proxyModel()->sort( -1 );
    m_detailedView->proxyModel()->sort( -1 );
    m_gridView->proxyModel()->sort( -1 );

    onModelChanged();
}


void
FlexibleView::setPlaylistModel( PlaylistModel* model )
{
    if ( m_model )
    {
        disconnect( m_model, SIGNAL( changed() ), this, SLOT( onModelChanged() ) );
    }

    setPlayableModel( model );

    connect( model, SIGNAL( changed() ), SLOT( onModelChanged() ), Qt::UniqueConnection );
}


void
FlexibleView::setCurrentMode( FlexibleViewMode mode )
{
    m_mode = mode;

    switch ( mode )
    {
        case Flat:
        {
            tDebug() << "m_trackView:" << m_trackView << m_stack->indexOf( m_trackView );
            m_stack->setCurrentWidget( m_trackView );
            break;
        }

        case Detailed:
        {
            m_stack->setCurrentWidget( m_detailedView );
            break;
        }

        case Grid:
        {
            m_stack->setCurrentWidget( m_gridView );
            break;
        }
    }

    emit modeChanged( mode );
}


Tomahawk::playlistinterface_ptr
FlexibleView::playlistInterface() const
{
    return m_trackView->proxyModel()->playlistInterface();
}


QString
FlexibleView::title() const
{
    return m_trackView->title();
}


QString
FlexibleView::description() const
{
    return m_trackView->description();
}


QPixmap
FlexibleView::pixmap() const
{
    return m_pixmap;
}


bool
FlexibleView::jumpToCurrentTrack()
{
    tDebug() << Q_FUNC_INFO;

    bool b = false;

    // note: the order of comparison is important here, if we'd write "b || foo" then foo will not be executed if b is already true!
    b = m_trackView->jumpToCurrentTrack() || b;
    b = m_detailedView->jumpToCurrentTrack() || b;
    b = m_gridView->jumpToCurrentTrack() || b;

    return b;
}


bool
FlexibleView::setFilter( const QString& pattern )
{
    ViewPage::setFilter( pattern );

    m_trackView->setFilter( pattern );
    m_detailedView->setFilter( pattern );
    m_gridView->setFilter( pattern );

    return true;
}


void
FlexibleView::setEmptyTip( const QString& tip )
{
    m_trackView->setEmptyTip( tip );
    m_detailedView->setEmptyTip( tip );
    m_gridView->setEmptyTip( tip );
}


void
FlexibleView::setPixmap( const QPixmap& pixmap )
{
    m_pixmap = pixmap;
    m_header->setPixmap( pixmap );
}


void
FlexibleView::onModelChanged()
{
    m_header->setPixmap( m_pixmap );
    m_header->setCaption( m_model->title() );
    m_header->setDescription( m_model->description() );

    if ( m_model->isReadOnly() )
        setEmptyTip( tr( "This playlist is currently empty." ) );
    else
        setEmptyTip( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );
}


void
FlexibleView::onWidgetDestroyed( QWidget* widget )
{
    Q_UNUSED( widget );
    emit destroyed( this );
}


bool
FlexibleView::isTemporaryPage() const
{
    return m_temporary;
}


void
FlexibleView::setTemporaryPage( bool b )
{
    m_temporary = b;
}


bool
FlexibleView::isBeingPlayed() const
{
    if ( !playlistInterface() )
        return false;

    if ( playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}
