/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "PlaylistView.h"

#include <QKeyEvent>
#include <QPainter>

#include "ViewManager.h"
#include "utils/Logger.h"
#include "PlaylistUpdaterInterface.h"
#include "Source.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : TrackView( parent )
    , m_model( 0 )
{
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

    TrackView::setPlayableModel( m_model );
    setColumnHidden( PlayableModel::Age, true ); // Hide age column per default
    setColumnHidden( PlayableModel::Composer, true ); // Hide composer column per default

    if ( guid().isEmpty() && proxyModel()->columnCount() > 1 )
    {
        if ( !m_model->playlist().isNull() )
        {
            setGuid( QString( "playlistview/%1/%2" ).arg( proxyModel()->columnCount() ).arg( m_model->playlist()->guid() ) );
        }
        else
        {
            setGuid( QString( "playlistview/%1" ).arg( proxyModel()->columnCount() ) );
        }
    }

    connect( m_model, SIGNAL( playlistDeleted() ), SLOT( onDeleted() ) );
    connect( m_model, SIGNAL( playlistChanged() ), SLOT( onChanged() ) );

    emit modelChanged();
}


void
PlaylistView::keyPressEvent( QKeyEvent* event )
{
    TrackView::keyPressEvent( event );

    if ( !model() )
        return;
}


bool
PlaylistView::eventFilter( QObject* obj, QEvent* event )
{
    if ( event->type() == QEvent::DragEnter )
    {
        QDragEnterEvent* e = static_cast<QDragEnterEvent*>(event);
        dragEnterEvent( e );
        return true;
    }
    if ( event->type() == QEvent::DragMove )
    {
        QDragMoveEvent* e = static_cast<QDragMoveEvent*>(event);
        dragMoveEvent( e );
        return true;
    }
    if ( event->type() == QEvent::DragLeave )
    {
        QDragLeaveEvent* e = static_cast<QDragLeaveEvent*>(event);
        dragLeaveEvent( e );
        return true;
    }
    if ( event->type() == QEvent::Drop )
    {
        QDropEvent* e = static_cast<QDropEvent*>(event);
        dropEvent( e );
        return true;
    }

    return QObject::eventFilter( obj, event );
}


QList<PlaylistUpdaterInterface*>
PlaylistView::updaters() const
{
    if ( !m_model->playlist().isNull() )
        return m_model->playlist()->updaters();

    return QList<PlaylistUpdaterInterface*>();
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
    if ( m_model )
    {
        if ( m_model->isReadOnly() )
            setEmptyTip( tr( "This playlist is currently empty." ) );
        else
            setEmptyTip( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );
        m_model->finishLoading();

        if ( !m_model->playlist().isNull() && ViewManager::instance()->currentPage() == this )
            emit nameChanged( m_model->playlist()->title() );
    }
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
        default:
            break;
    }
}
