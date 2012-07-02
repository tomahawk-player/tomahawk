/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "playlist/PlayableModel.h"
#include "playlist/TrackView.h"
#include "playlist/GridView.h"
#include "playlist/PlaylistLargeItemDelegate.h"
#include "utils/Closure.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


FlexibleView::FlexibleView( QWidget* parent )
    : QWidget( parent )
    , m_trackView( new TrackView() )
    , m_detailedView( new TrackView() )
    , m_gridView( new GridView() )
    , m_model( 0 )
{
    qRegisterMetaType< FlexibleViewMode >( "FlexibleViewMode" );

    PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::LovedTracks, m_trackView, m_trackView->proxyModel() );
    connect( del, SIGNAL( updateIndex( QModelIndex ) ), m_trackView, SLOT( update( QModelIndex ) ) );
    m_trackView->setItemDelegate( del );
    m_trackView->proxyModel()->setStyle( PlayableProxyModel::Large );

    m_stack = new QStackedWidget();
    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    QWidget* modeBar = new QWidget();
    modeBar->setLayout( new QHBoxLayout() );
    TomahawkUtils::unmarginLayout( modeBar->layout() );

    QWidget* modeWidget = new QWidget();
    modeWidget->setLayout( new QHBoxLayout() );
    modeWidget->setFixedSize( QSize( 87, 30 ) );
    TomahawkUtils::unmarginLayout( modeWidget->layout() );

    QRadioButton* radioNormal = new QRadioButton();
    radioNormal->setObjectName( "radioNormal" );
    QRadioButton* radioDetailed = new QRadioButton();
    radioDetailed->setObjectName( "radioDetailed" );
    QRadioButton* radioCloud = new QRadioButton();
    radioCloud->setObjectName( "radioCloud" );

    radioNormal->setFocusPolicy( Qt::NoFocus );
    radioDetailed->setFocusPolicy( Qt::NoFocus );
    radioCloud->setFocusPolicy( Qt::NoFocus );

    QFile f( RESPATH "stylesheets/topbar-radiobuttons.css" );
    f.open( QFile::ReadOnly );
    QString css = QString::fromAscii( f.readAll() );
    f.close();

    modeWidget->setStyleSheet( css );
    modeWidget->layout()->addWidget( radioNormal );
    modeWidget->layout()->addWidget( radioDetailed );
    modeWidget->layout()->addWidget( radioCloud );
    modeWidget->layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed ) );

    modeBar->layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    modeBar->layout()->addWidget( modeWidget );
    modeBar->layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed ) );

    layout()->addWidget( modeBar );
    layout()->addWidget( m_stack );

    m_stack->addWidget( m_trackView );
    m_stack->addWidget( m_detailedView );
    m_stack->addWidget( m_gridView );

    radioNormal->setChecked( true );
    setCurrentMode( Flat );

    NewClosure( radioNormal,   SIGNAL( clicked() ), const_cast< FlexibleView* >( this ), SLOT( setCurrentMode( FlexibleViewMode ) ), Flat )->setAutoDelete( false );
    NewClosure( radioDetailed, SIGNAL( clicked() ), const_cast< FlexibleView* >( this ), SLOT( setCurrentMode( FlexibleViewMode ) ), Detailed )->setAutoDelete( false );
    NewClosure( radioCloud,    SIGNAL( clicked() ), const_cast< FlexibleView* >( this ), SLOT( setCurrentMode( FlexibleViewMode ) ), Grid )->setAutoDelete( false );
}


FlexibleView::~FlexibleView()
{
    tDebug() << Q_FUNC_INFO;
}


void
FlexibleView::setTrackView( TrackView* view )
{
    if ( m_trackView )
    {
        delete m_trackView;
    }

    m_trackView = view;
    m_stack->addWidget( view );
}


void
FlexibleView::setDetailedView( TrackView* view )
{
    if ( m_detailedView )
    {
        delete m_detailedView;
    }

    m_detailedView = view;
    m_stack->addWidget( view );
}


void
FlexibleView::setGridView( GridView* view )
{
    if ( m_gridView )
    {
        delete m_gridView;
    }

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
}


void
FlexibleView::setCurrentMode( FlexibleViewMode mode )
{
    m_mode = mode;

    switch ( mode )
    {
        case Flat:
        {
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
    return m_trackView->playlistInterface();
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
    return m_trackView->pixmap();
}


bool
FlexibleView::jumpToCurrentTrack()
{
    m_trackView->jumpToCurrentTrack();
    m_detailedView->jumpToCurrentTrack();
    m_gridView->jumpToCurrentTrack();
    return true;
}
