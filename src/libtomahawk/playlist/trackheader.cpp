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

#include "trackheader.h"

#include <QContextMenuEvent>
#include <QMenu>

#include "tomahawksettings.h"
#include "playlist/trackmodel.h"
#include "playlist/trackview.h"
#include "utils/logger.h"


TrackHeader::TrackHeader( TrackView* parent )
    : QHeaderView( Qt::Horizontal, parent )
    , m_parent( parent )
    , m_menu( new QMenu( this ) )
    , m_sigmap( new QSignalMapper( this ) )
    , m_init( false )
{
    setResizeMode( QHeaderView::Interactive );
    setMinimumSectionSize( 60 );
    setDefaultAlignment( Qt::AlignLeft );
    setMovable( true );
    setStretchLastSection( true );
//    setCascadingSectionResizes( true );

//    m_menu->addAction( tr( "Resize columns to fit window" ), this, SLOT( onToggleResizeColumns() ) );
//    m_menu->addSeparator();

    connect( this, SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized() ) );
    connect( m_sigmap, SIGNAL( mapped( int ) ), SLOT( toggleVisibility( int ) ) );
}


TrackHeader::~TrackHeader()
{
}


void
TrackHeader::onSectionResized()
{
    if ( !m_init )
        return;

    TomahawkSettings::instance()->setPlaylistColumnSizes( m_parent->guid(), saveState() );
}


int
TrackHeader::visibleSectionCount() const
{
    return count() - hiddenSectionCount();
}


bool
TrackHeader::checkState()
{
    if ( !count() || m_init )
        return false;

    QByteArray state = TomahawkSettings::instance()->playlistColumnSizes( m_parent->guid() );
    if ( !state.isEmpty() )
    {
        restoreState( state );

        if ( m_parent->guid().startsWith( "playlistview" ) ) // HACK
            setSortIndicator( -1, Qt::AscendingOrder );
    }
    else
    {
        QList< double > m_columnWeights;
        m_columnWeights << 0.18 << 0.18 << 0.17 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.10; // << 0.05;

        for ( int i = 0; i < count() - 1; i++ )
        {
            if ( isSectionHidden( i ) )
                continue;

            double nw = (double)m_parent->width() * m_columnWeights.at( i );
            qDebug() << "Setting default size:" << i << nw;
            resizeSection( i, qMax( minimumSectionSize(), int( nw - 0.5 ) ) );
        }
    }

    m_init = true;
    return true;
}


void
TrackHeader::addColumnToMenu( int index )
{
    QString title = m_parent->model()->headerData( index, Qt::Horizontal, Qt::DisplayRole ).toString();

    QAction* action = m_menu->addAction( title, m_sigmap, SLOT( map() ) );
    action->setCheckable( true );
    action->setChecked( !isSectionHidden( index ) );
    m_visActions << action;

    m_sigmap->setMapping( action, index );
}


void
TrackHeader::contextMenuEvent( QContextMenuEvent* e )
{
    qDeleteAll( m_visActions );
    m_visActions.clear();

    for ( int i = 0; i < count(); i++ )
        addColumnToMenu( i );

    m_menu->popup( e->globalPos() );
}


void
TrackHeader::onToggleResizeColumns()
{
}


void
TrackHeader::toggleVisibility( int index )
{
    qDebug() << Q_FUNC_INFO << index;

    if ( isSectionHidden( index ) )
        showSection( index );
    else
        hideSection( index );
}
