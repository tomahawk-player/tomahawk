/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2014, Teo Mrnjavac <teo@kde.org>
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

#include "FlexibleTreeView.h"

#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "audio/AudioEngine.h"
#include "widgets/FilterHeader.h"
#include "playlist/TreeModel.h"
#include "playlist/ColumnView.h"
#include "playlist/TrackView.h"
#include "playlist/TreeView.h"
#include "playlist/GridView.h"
#include "playlist/ModeHeader.h"
#include "playlist/PlaylistLargeItemDelegate.h"
#include "PlayableProxyModelPlaylistInterface.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

using namespace Tomahawk;


FlexibleTreeView::FlexibleTreeView( QWidget* parent, QWidget* extraHeader )
    : QWidget( parent )
    , m_header( new FilterHeader( this ) )
    , m_modeHeader( new ModeHeader( this ) )
    , m_columnView( new ColumnView() )
    , m_treeView( new TreeView() )
    , m_trackView( 0 )
    , m_model( 0 )
    , m_temporary( false )
{
    qRegisterMetaType< FlexibleTreeViewMode >( "FlexibleTreeViewMode" );

    m_treeView->proxyModel()->setStyle( PlayableProxyModel::Collection );

    m_treeView->proxyModel()->setPlaylistInterface( m_columnView->proxyModel()->playlistInterface() );

//    m_trackView->setPlaylistInterface( m_playlistInterface );
//    m_columnView->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );
//    m_gridView->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );

/*    m_columnView->setColumnHidden( PlayableModel::Age, true ); // Hide age column per default
    m_columnView->setColumnHidden( PlayableModel::Filesize, true ); // Hide filesize column per default
    m_columnView->setColumnHidden( PlayableModel::Composer, true ); // Hide composer column per default*/

/*    PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::LovedTracks, m_trackView, m_trackView->proxyModel() );
    m_trackView->setPlaylistItemDelegate( del );
    m_trackView->proxyModel()->setStyle( PlayableProxyModel::Large );*/

    m_stack = new QStackedWidget();
    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    QFrame* lineBelow = new QFrame( this );
    lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );
    lineBelow->setFrameShape( QFrame::HLine );
    lineBelow->setMaximumHeight( 1 );
    QFrame* lineBelow2 = new QFrame( this );
    lineBelow2->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    lineBelow2->setFrameShape( QFrame::HLine );
    lineBelow2->setMaximumHeight( 1 );

    layout()->addWidget( m_header );
    layout()->addWidget( m_modeHeader );
    if ( extraHeader )
        layout()->addWidget( extraHeader );
    layout()->addWidget( lineBelow );
    layout()->addWidget( lineBelow2 );
    layout()->addWidget( m_stack );

    m_stack->addWidget( m_columnView );
    m_stack->addWidget( m_treeView );
    /*    m_stack->addWidget( m_gridView );
    m_stack->addWidget( m_trackView );*/

    connect( m_header, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );

    NewClosure( m_modeHeader, SIGNAL( flatClicked() ), const_cast< FlexibleTreeView* >( this ), SLOT( setCurrentMode( FlexibleTreeViewMode ) ), FlexibleTreeView::Columns )->setAutoDelete( false );
    NewClosure( m_modeHeader, SIGNAL( detailedClicked() ), const_cast< FlexibleTreeView* >( this ), SLOT( setCurrentMode( FlexibleTreeViewMode ) ), FlexibleTreeView::Flat )->setAutoDelete( false );
    NewClosure( m_modeHeader, SIGNAL( gridClicked() ), const_cast< FlexibleTreeView* >( this ), SLOT( setCurrentMode( FlexibleTreeViewMode ) ), FlexibleTreeView::Albums )->setAutoDelete( false );
}


FlexibleTreeView::~FlexibleTreeView()
{
    tDebug() << Q_FUNC_INFO;
}


void
FlexibleTreeView::setGuid( const QString& guid )
{
    m_treeView->setGuid( guid );
    m_columnView->setGuid( guid );
}


void
FlexibleTreeView::setTrackView( TrackView* view )
{
    if ( m_trackView )
    {
        m_stack->removeWidget( m_trackView );
        delete m_trackView;
    }

//    view->setPlaylistInterface( m_playlistInterface );

    m_trackView = view;
    m_stack->addWidget( view );
}


void
FlexibleTreeView::setColumnView( ColumnView* view )
{
    if ( m_columnView )
    {
        m_stack->removeWidget( m_columnView );
        delete m_columnView;
    }

    connect( view, SIGNAL( destroyed( QWidget* ) ), SLOT( onWidgetDestroyed( QWidget* ) ), Qt::UniqueConnection );

//    view->setPlaylistInterface( m_trackView->proxyModel()->playlistInterface() );

    m_columnView = view;
    m_stack->addWidget( view );
}


void
FlexibleTreeView::setTreeView( TreeView* view )
{
    if ( m_treeView )
    {
        m_stack->removeWidget( m_treeView );
        delete m_treeView;
    }

//    view->setPlaylistInterface( m_columnView->proxyModel()->playlistInterface() );

    m_treeView = view;
    m_stack->addWidget( view );
}


void
FlexibleTreeView::setTreeModel( TreeModel* model )
{
    if ( m_model )
    {
        disconnect( m_model, SIGNAL( changed() ), this, SLOT( onModelChanged() ) );
        delete m_model;
    }

    m_model = model;

//    m_trackView->setPlayableModel( model );
    m_columnView->setTreeModel( model );
    m_treeView->setTreeModel( model );
    //    m_gridView->setPlayableModel( model );

/*    m_trackView->setSortingEnabled( false );
    m_trackView->sortByColumn( -1 );
    m_trackView->proxyModel()->sort( -1 );
    m_columnView->proxyModel()->sort( -1 );
    m_gridView->proxyModel()->sort( -1 );*/

    connect( model, SIGNAL( changed() ), SLOT( onModelChanged() ), Qt::UniqueConnection );
    onModelChanged();
}


void
FlexibleTreeView::setCurrentMode( FlexibleTreeViewMode mode )
{
    if ( m_mode != mode )
    {
        TomahawkSettings::instance()->beginGroup( "ui" );
        TomahawkSettings::instance()->setValue( "flexibleTreeViewMode", mode );
        TomahawkSettings::instance()->endGroup();
        TomahawkSettings::instance()->sync();

        m_mode = mode;
    }

    switch ( mode )
    {
        case Flat:
        {
            m_stack->setCurrentWidget( m_treeView );
            break;
        }

        case Columns:
        {
            m_stack->setCurrentWidget( m_columnView );
            break;
        }

        case Albums:
        {
//            m_stack->setCurrentWidget( m_gridView );
            break;
        }
    }

    emit modeChanged( mode );
}


Tomahawk::playlistinterface_ptr
FlexibleTreeView::playlistInterface() const
{
    return m_columnView->proxyModel()->playlistInterface();
}


QString
FlexibleTreeView::title() const
{
    return m_model->title();
}


QString
FlexibleTreeView::description() const
{
    return m_model->description();
}


QPixmap
FlexibleTreeView::pixmap() const
{
    return m_pixmap;
}


bool
FlexibleTreeView::jumpToCurrentTrack()
{
    tDebug() << Q_FUNC_INFO;

    bool b = false;

    // note: the order of comparison is important here, if we'd write "b || foo" then foo will not be executed if b is already true!
    b = m_columnView->jumpToCurrentTrack() || b;
//    b = m_trackView->jumpToCurrentTrack() || b;
    b = m_treeView->jumpToCurrentTrack() || b;

    return b;
}


bool
FlexibleTreeView::setFilter( const QString& pattern )
{
    ViewPage::setFilter( pattern );

    m_columnView->setFilter( pattern );
    m_treeView->proxyModel()->setFilter( pattern );
    /*    m_gridView->setFilter( pattern );
    m_trackView->setFilter( pattern );*/

    return true;
}


void
FlexibleTreeView::restoreViewMode()
{
    TomahawkSettings::instance()->beginGroup( "ui" );
    int modeNumber = TomahawkSettings::instance()->value( "flexibleTreeViewMode", Columns ).toInt();
    m_mode = static_cast< FlexibleTreeViewMode >( modeNumber );
    TomahawkSettings::instance()->endGroup();

    switch ( m_mode )
    {
    case Columns:
        m_modeHeader->switchTo( 0 );
        break;
    case Flat:
        m_modeHeader->switchTo( 1 );
        break;
    case Albums:
        m_modeHeader->switchTo( 2 );
    }
}


void
FlexibleTreeView::setEmptyTip( const QString& tip )
{
    m_columnView->setEmptyTip( tip );
    m_treeView->setEmptyTip( tip );
    /*    m_gridView->setEmptyTip( tip );
    m_trackView->setEmptyTip( tip );*/
}


void
FlexibleTreeView::setPixmap( const QPixmap& pixmap )
{
    m_pixmap = pixmap;
    m_header->setPixmap( pixmap );
}


void
FlexibleTreeView::onModelChanged()
{
    setPixmap( m_model->icon() );
    m_header->setCaption( m_model->title() );
    m_header->setDescription( m_model->description() );

    if ( m_model->isReadOnly() )
        setEmptyTip( tr( "This playlist is currently empty." ) );
    else
        setEmptyTip( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );
}


void
FlexibleTreeView::onWidgetDestroyed( QWidget* widget )
{
    Q_UNUSED( widget );
    emit destroyed( this );
}


bool
FlexibleTreeView::isTemporaryPage() const
{
    return m_temporary;
}


void
FlexibleTreeView::setTemporaryPage( bool b )
{
    m_temporary = b;
}


bool
FlexibleTreeView::isBeingPlayed() const
{
    if ( !playlistInterface() )
        return false;

    if ( playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}
