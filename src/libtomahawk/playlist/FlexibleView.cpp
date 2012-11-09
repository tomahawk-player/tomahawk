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

#include "playlist/FlexibleHeader.h"
#include "playlist/PlayableModel.h"
#include "playlist/TrackView.h"
#include "playlist/GridView.h"
#include "playlist/PlaylistLargeItemDelegate.h"
#include "PlayableProxyModelPlaylistInterface.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


class FlexibleViewInterface : public PlayableProxyModelPlaylistInterface
{
    Q_OBJECT
public:
    explicit FlexibleViewInterface( PlayableProxyModel* proxy, FlexibleView* view )
        : PlayableProxyModelPlaylistInterface( proxy )
        , m_view( view )
    {
    }

    virtual bool hasChildInterface( playlistinterface_ptr playlistInterface )
    {
        if ( m_view.isNull() )
            return false;

        if ( m_view.data()->detailedView() && m_view.data()->detailedView()->proxyModel()->playlistInterface() == playlistInterface )
            return true;

        if ( m_view.data()->gridView() && m_view.data()->gridView()->playlistInterface()->hasChildInterface( playlistInterface ) )
            return true;

        if ( m_view.data()->trackView() && m_view.data()->trackView()->proxyModel()->playlistInterface() == playlistInterface )
            return true;

        return false;
    }

private:
    QWeakPointer<FlexibleView> m_view;
};


FlexibleView::FlexibleView( QWidget* parent )
    : QWidget( parent )
    , m_header( new FlexibleHeader( this ) )
    , m_trackView( new TrackView() )
    , m_detailedView( new TrackView() )
    , m_gridView( new GridView() )
    , m_model( 0 )
{
    qRegisterMetaType< FlexibleViewMode >( "FlexibleViewMode" );

    m_playlistInterface = playlistinterface_ptr( new FlexibleViewInterface( m_trackView->proxyModel(), this ) );
    m_trackView->setPlaylistInterface( m_playlistInterface );
    m_detailedView->setPlaylistInterface( m_playlistInterface );

    m_detailedView->setColumnHidden( PlayableModel::Age, true ); // Hide age column per default
    m_detailedView->setColumnHidden( PlayableModel::Composer, true ); // Hide composer column per default

    PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::LovedTracks, m_trackView, m_trackView->proxyModel() );
    connect( del, SIGNAL( updateIndex( QModelIndex ) ), m_trackView, SLOT( update( QModelIndex ) ) );
    m_trackView->setItemDelegate( del );
    m_trackView->proxyModel()->setStyle( PlayableProxyModel::Large );

    m_stack = new QStackedWidget();
    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    layout()->addWidget( m_header );
    layout()->addWidget( m_stack );

    m_stack->addWidget( m_trackView );
    m_stack->addWidget( m_detailedView );
    m_stack->addWidget( m_gridView );

    setCurrentMode( Flat );

    connect( m_header, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );
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
        m_stack->removeWidget( m_trackView );
        delete m_trackView;
    }

    view->setPlaylistInterface( m_playlistInterface );

    m_trackView = view;
    m_stack->addWidget( view );
}


void
FlexibleView::setDetailedView( TrackView* view )
{
    if ( m_detailedView )
    {
        m_stack->removeWidget( m_detailedView );
        delete m_detailedView;
    }

    view->setPlaylistInterface( m_playlistInterface );

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

    m_header->setPixmap( m_pixmap );
    m_header->setCaption( model->title() );
    m_header->setDescription( model->description() );
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
    return m_playlistInterface;
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
    m_trackView->jumpToCurrentTrack();
    m_detailedView->jumpToCurrentTrack();
    m_gridView->jumpToCurrentTrack();
    return true;
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

#include "FlexibleView.moc"
