/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "HistoryWidget.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "MetaPlaylistInterface.h"

#include "playlist/RecentlyPlayedModel.h"
#include "playlist/TrackView.h"
#include "playlist/PlaylistLargeItemDelegate.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QCalendarWidget>
#include <QDateEdit>
#include <QHBoxLayout>

using namespace Tomahawk;

HistoryWidget::HistoryWidget( const source_ptr& source, QWidget* parent )
    : FlexibleView( parent, m_header = new QWidget() )
{
    m_header->setMaximumHeight( 160 );
/*    QCalendarWidget* m_calendarFrom = new QCalendarWidget();
    QCalendarWidget* m_calendarTo = new QCalendarWidget();
    m_calendarFrom->setGridVisible( false );
    m_calendarTo->setGridVisible( false );*/
    m_calendarFrom = new QDateEdit( QDate::currentDate() );
    m_calendarTo = new QDateEdit( QDate::currentDate() );
    m_calendarFrom->setDisplayFormat( "yyyy MMMM dd" );
    m_calendarTo->setDisplayFormat( "yyyy MMMM dd" );

    QHBoxLayout* layout = new QHBoxLayout( m_header );
    layout->addWidget( m_calendarFrom );
    layout->addWidget( m_calendarTo );
    m_header->setLayout( layout );

    setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RecentlyPlayed ) );

    m_model = new RecentlyPlayedModel( this );
    m_model->setTitle( tr( "Recently Played Tracks" ) );

    if ( source->isLocal() )
        m_model->setDescription( tr( "Your recently played tracks" ) );
    else
        m_model->setDescription( tr( "%1's recently played tracks" ).arg( source->friendlyName() ) );

    PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::RecentlyPlayed, trackView(), trackView()->proxyModel() );
    trackView()->setItemDelegate( del );

    setPlayableModel( m_model );
    setEmptyTip( tr( "Sorry, we could not find any recent plays!" ) );
    m_model->setSource( source );

    setGuid( QString( "recentplays/%1" ).arg( source->nodeId() ) );

/*    connect( m_calendarFrom, SIGNAL( clicked( QDate ) ), SLOT( onDateClicked( QDate ) ) );
    connect( m_calendarTo, SIGNAL( clicked( QDate ) ), SLOT( onDateClicked( QDate ) ) );*/
    connect( m_calendarFrom, SIGNAL( dateChanged( QDate ) ), SLOT( onDateClicked( QDate ) ) );
    connect( m_calendarTo, SIGNAL( dateChanged( QDate ) ), SLOT( onDateClicked( QDate ) ) );
}


HistoryWidget::~HistoryWidget()
{
}


void
HistoryWidget::onDateClicked( const QDate& date )
{
    QDateEdit* cw = qobject_cast< QDateEdit* >( sender() );
    if ( cw == m_calendarFrom )
    {
        m_calendarTo->setDate( date );
    }

    m_model->setLimit( 0 );
    m_model->setDateFrom( m_calendarFrom->date() );
    m_model->setDateTo( m_calendarTo->date() );
}
