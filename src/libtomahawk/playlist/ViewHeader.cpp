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

#include "ViewHeader.h"

#include <QContextMenuEvent>
#include <QMenu>

#include "TomahawkSettings.h"
#include "utils/Logger.h"


ViewHeader::ViewHeader( QAbstractItemView* parent )
    : QHeaderView( Qt::Horizontal, parent )
    , m_parent( parent )
    , m_menu( new QMenu( this ) )
    , m_sigmap( new QSignalMapper( this ) )
    , m_init( false )
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    setSectionResizeMode( QHeaderView::Interactive );
    setSectionsMovable( true );
#else
    setResizeMode( QHeaderView::Interactive );
    setMovable( true );
#endif
    setMinimumSectionSize( 60 );
    setDefaultAlignment( Qt::AlignLeft );
    setStretchLastSection( true );

//    m_menu->addAction( tr( "Resize columns to fit window" ), this, SLOT( onToggleResizeColumns() ) );
//    m_menu->addSeparator();

    connect( m_sigmap, SIGNAL( mapped( int ) ), SLOT( toggleVisibility( int ) ) );
}


ViewHeader::~ViewHeader()
{
}


int
ViewHeader::visibleSectionCount() const
{
    return count() - hiddenSectionCount();
}


void
ViewHeader::onSectionsChanged()
{
    tDebug( LOGVERBOSE ) << "Saving columns state for view guid:" << m_guid;
    if ( !m_guid.isEmpty() )
        TomahawkSettings::instance()->setPlaylistColumnSizes( m_guid, saveState() );
}


bool
ViewHeader::checkState()
{
    if ( !count() || m_init )
        return false;

    disconnect( this, SIGNAL( sectionMoved( int, int, int ) ), this, SLOT( onSectionsChanged() ) );
    disconnect( this, SIGNAL( sectionResized( int, int, int ) ), this, SLOT( onSectionsChanged() ) );

    QByteArray state;
    tDebug( LOGVERBOSE ) << "Restoring columns state for view:" << m_guid;

    if ( !m_guid.isEmpty() )
        state = TomahawkSettings::instance()->playlistColumnSizes( m_guid );

    if ( !state.isEmpty() )
    {
        restoreState( state );
    }
    else
    {
        tDebug( LOGVERBOSE ) << "Giving columns initial weighting:" << m_columnWeights;
        for ( int i = 0; i < count() - 1; i++ )
        {
            if ( isSectionHidden( i ) )
                continue;
            if ( i >= m_columnWeights.count() )
                break;

            double nw = (double)m_parent->width() * m_columnWeights.at( i );
            resizeSection( i, qMax( minimumSectionSize(), int( nw - 0.5 ) ) );
        }
    }

    connect( this, SIGNAL( sectionMoved( int, int, int ) ), SLOT( onSectionsChanged() ) );
    connect( this, SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionsChanged() ) );

    m_init = true;
    return true;
}


void
ViewHeader::addColumnToMenu( int index )
{
    QString title = m_parent->model()->headerData( index, Qt::Horizontal, Qt::DisplayRole ).toString();

    QAction* action = m_menu->addAction( title, m_sigmap, SLOT( map() ) );
    action->setCheckable( true );
    action->setChecked( !isSectionHidden( index ) );
    m_visActions << action;

    m_sigmap->setMapping( action, index );
}


void
ViewHeader::contextMenuEvent( QContextMenuEvent* e )
{
    qDeleteAll( m_visActions );
    m_visActions.clear();

    for ( int i = 0; i < count(); i++ )
        addColumnToMenu( i );

    m_menu->popup( e->globalPos() );
}


void
ViewHeader::onToggleResizeColumns()
{
}


void
ViewHeader::toggleVisibility( int index )
{
    if ( isSectionHidden( index ) )
        showSection( index );
    else
        hideSection( index );
}


void ViewHeader::setGuid( const QString& guid )
{
    m_guid = guid;

    // If we are _not_ initialized yet, this means we're still waiting for the view to resize initially.
    // We need to wait with restoring our state (column sizes) until that has happened.
    if ( m_init )
    {
        m_init = false;
        checkState();
    }
}
