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

#include "playlistview.h"

#include <QKeyEvent>
#include <QPainter>

#include "playlist/playlistproxymodel.h"
#include "widgets/overlaywidget.h"
#include "viewmanager.h"
#include "utils/logger.h"
#include "PlaylistUpdaterInterface.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : TrackView( parent )
    , m_model( 0 )
{
    setProxyModel( new PlaylistProxyModel( this ) );

    connect( contextMenu(), SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;
}


void
PlaylistView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setPlaylistModel instead";
    Q_ASSERT( false );
}


void
PlaylistView::setPlaylistModel( PlaylistModel* model )
{
    m_model = model;

    TrackView::setTrackModel( m_model );
    setColumnHidden( TrackModel::Age, true ); // Hide age column per default
    setColumnHidden( TrackModel::Composer, true ); // Hide composer column per default

    if ( guid().isEmpty() )
    {
        if ( !m_model->playlist().isNull() )
        {
            setGuid( QString( "playlistview/%1/%2" ).arg( m_model->columnCount() ).arg( m_model->playlist()->guid() ) );
        }
        else
        {
            setGuid( QString( "playlistview/%1" ).arg( m_model->columnCount() ) );
        }
    }

    connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), SLOT( onTrackCountChanged( unsigned int ) ) );
    connect( m_model, SIGNAL( playlistDeleted() ), SLOT( onDeleted() ) );
    connect( m_model, SIGNAL( playlistChanged() ), SLOT( onChanged() ) );
}


void
PlaylistView::keyPressEvent( QKeyEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    TrackView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( ( event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace ) && !model()->isReadOnly() )
    {
        qDebug() << "Removing selected items";
        proxyModel()->remove( selectedIndexes() );
    }
}


void
PlaylistView::deleteItems()
{
    proxyModel()->remove( selectedIndexes() );
}


bool
PlaylistView::canAutoUpdate() const
{
    if ( !m_model->playlist().isNull() && m_model->playlist()->updater() )
        return true;

    return false;
}


bool
PlaylistView::autoUpdate() const
{
    if ( canAutoUpdate() )
        return m_model->playlist()->updater()->autoUpdate();

    return false;
}


void
PlaylistView::setAutoUpdate( bool autoUpdate )
{
    if ( !canAutoUpdate() )
        return;

    m_model->playlist()->updater()->setAutoUpdate( autoUpdate );
}


void
PlaylistView::onTrackCountChanged( unsigned int tracks )
{
    if ( tracks == 0 )
    {
        overlay()->setText( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );
        overlay()->show();
    }
    else
        overlay()->hide();
}


bool
PlaylistView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}


void
PlaylistView::onDeleted()
{
    qDebug() << Q_FUNC_INFO;
    emit destroyed( widget() );
}


void
PlaylistView::onChanged()
{
    if ( m_model && !m_model->playlist().isNull() &&
         ViewManager::instance()->currentPage() == this )
        emit nameChanged( m_model->playlist()->title() );
}


bool
PlaylistView::isTemporaryPage() const
{
    return ( m_model && m_model->isTemporary() );
}


void
PlaylistView::onMenuTriggered( int action )
{
    switch ( action )
    {
        case ContextMenu::ActionDelete:
            deleteItems();
            break;

        default:
            TrackView::onMenuTriggered( action );
            break;
    }
}
