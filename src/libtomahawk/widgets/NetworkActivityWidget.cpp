/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "NetworkActivityWidget_p.h"

#include "Pipeline.h"
#include "audio/AudioEngine.h"
#include "database/Database.h"
#include "database/DatabaseCommand_NetworkCharts.h"
#include "playlist/PlaylistChartItemDelegate.h"
#include "utils/AnimatedSpinner.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"

#include <QDateTime>
#include <QStandardItemModel>
#include <QtConcurrentRun>

#define NETWORKCHARTS_NUM_TRACKS 100

using namespace Tomahawk;

NetworkActivityWidget::NetworkActivityWidget( QWidget* parent )
    : QWidget( parent )
    , d_ptr( new NetworkActivityWidgetPrivate ( this ) )
{
    d_func()->ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( d_func()->ui->stackLeft->layout() );
    TomahawkUtils::unmarginLayout( d_func()->ui->verticalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( d_func()->ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( d_func()->ui->breadCrumbLeft->layout() );

    d_func()->crumbModelLeft = new QStandardItemModel( this );
    d_func()->sortedProxy = new QSortFilterProxyModel( this );
    d_func()->sortedProxy->setDynamicSortFilter( true );
    d_func()->sortedProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );

    d_func()->ui->breadCrumbLeft->setRootIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::NetworkActivity, TomahawkUtils::Original ) );
    connect( d_func()->ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged( QModelIndex ) ) );

    d_func()->ui->tracksViewLeft->setHeaderHidden( true );
    d_func()->ui->tracksViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    PlaylistChartItemDelegate* del = new PlaylistChartItemDelegate( d_func()->ui->tracksViewLeft, d_func()->ui->tracksViewLeft->proxyModel() );
    d_func()->ui->tracksViewLeft->setItemDelegate( del );
    d_func()->ui->tracksViewLeft->setUniformRowHeights( false );

    d_func()->playlistInterface = d_func()->ui->tracksViewLeft->playlistInterface();

    // Build up breadcrumb
    QStandardItem* rootItem = d_func()->crumbModelLeft->invisibleRootItem();
    QStandardItem* chartItem = new QStandardItem( tr( "Charts" ) );
    rootItem->appendRow( chartItem );
    QStandardItem* weekItem = new QStandardItem( tr( "Last Week" ) );
    weekItem->setData( WeekChart, Breadcrumb::DefaultRole );
    chartItem->appendRow( weekItem );
    QStandardItem* monthItem = new QStandardItem( tr( "Last Month" ) );
    monthItem->setData( MonthChart, Breadcrumb::DefaultRole );
    chartItem->appendRow( monthItem );
    QStandardItem* yearItem = new QStandardItem( tr( "Last Year" ) );
    yearItem->setData( YearChart, Breadcrumb::DefaultRole );
    chartItem->appendRow( yearItem );
    QStandardItem* overallItem = new QStandardItem( tr( "Overall" ) );
    overallItem->setData( OverallChart, Breadcrumb::DefaultRole );
    chartItem->appendRow( overallItem );
    d_func()->sortedProxy->setSourceModel( d_func()->crumbModelLeft );
    d_func()->sortedProxy->sort( 0, Qt::AscendingOrder );
    d_func()->ui->breadCrumbLeft->setModel( d_func()->sortedProxy );
    d_func()->ui->breadCrumbLeft->setVisible( true );
}


NetworkActivityWidget::~NetworkActivityWidget()
{
    delete d_ptr;
}


Tomahawk::playlistinterface_ptr
NetworkActivityWidget::playlistInterface() const
{
    return d_func()->playlistInterface;
}


bool
NetworkActivityWidget::isBeingPlayed() const
{
    if ( AudioEngine::instance()->currentTrackPlaylist() == d_func()->ui->tracksViewLeft->playlistInterface() )
        return true;

    return false;
}


bool
NetworkActivityWidget::jumpToCurrentTrack()
{
    if ( d_func()->ui->tracksViewLeft->model() && d_func()->ui->tracksViewLeft->jumpToCurrentTrack() )
        return true;

    return false;
}


void
NetworkActivityWidget::weeklyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    d_func()->weeklyChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->weeklyChartsModel->startLoading();
    d_func()->weeklyChartsModel->appendTracks( tracks );
    d_func()->weeklyChartsModel->finishLoading();

    if ( d_func()->activeView == WeekChart )
    {
        showWeekCharts();
    }
}


void
NetworkActivityWidget::monthlyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    d_func()->monthlyChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->monthlyChartsModel->startLoading();
    d_func()->monthlyChartsModel->appendTracks( tracks );
    d_func()->monthlyChartsModel->finishLoading();

    if ( d_func()->activeView == MonthChart )
    {
        showMonthCharts();
    }
}


void
NetworkActivityWidget::yearlyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    d_func()->yearlyChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->yearlyChartsModel->startLoading();
    d_func()->yearlyChartsModel->appendTracks( tracks );
    d_func()->yearlyChartsModel->finishLoading();

    if ( d_func()->activeView == YearChart )
    {
        showYearCharts();
    }
}

void
NetworkActivityWidget::overallCharts( const QList<track_ptr>& tracks )
{
    d_func()->overallChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->overallChartsModel->startLoading();
    d_func()->overallChartsModel->appendTracks( tracks );
    d_func()->overallChartsModel->finishLoading();

    if ( d_func()->activeView == OverallChart )
    {
        showOverallCharts();
    }
}


void
NetworkActivityWidget::leftCrumbIndexChanged( const QModelIndex& index )
{
    QStandardItem* item = d_func()->crumbModelLeft->itemFromIndex( d_func()->sortedProxy->mapToSource( index ) );
    if ( !item )
        return;
    if ( !item->data( Breadcrumb::DefaultRole ).isValid() )
        return;

    int chartId = item->data( Breadcrumb::DefaultRole ).toInt();
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Showing chart" << chartId;
    switch ( chartId )
    {
    case WeekChart:
        showWeekCharts();
        break;
    case MonthChart:
        showMonthCharts();
        break;
    case YearChart:
        showYearCharts();
        break;
    case OverallChart:
        showOverallCharts();
        break;
    }
}


void
NetworkActivityWidget::fetchYearCharts()
{
    QDateTime to = QDateTime::currentDateTime();
    QDateTime yearAgo = to.addYears( -1 );
    DatabaseCommand_NetworkCharts* yearCharts = new DatabaseCommand_NetworkCharts( yearAgo, to );
    yearCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( yearCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( yearlyCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( yearCharts ) );
}

void
NetworkActivityWidget::fetchOverallCharts()
{
    DatabaseCommand_NetworkCharts* overallCharts = new DatabaseCommand_NetworkCharts();
    overallCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( overallCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( overallCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( overallCharts ) );
}


void
NetworkActivityWidget::fetchWeekCharts()
{
    QDateTime to = QDateTime::currentDateTime();
    QDateTime weekAgo = to.addDays( -7 );
    DatabaseCommand_NetworkCharts* weekCharts = new DatabaseCommand_NetworkCharts( weekAgo, to );
    weekCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( weekCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( weeklyCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( weekCharts ) );
}

void
NetworkActivityWidget::fetchMonthCharts()
{
    QDateTime to = QDateTime::currentDateTime();
    QDateTime monthAgo = to.addMonths( -1 );
    DatabaseCommand_NetworkCharts* monthCharts = new DatabaseCommand_NetworkCharts( monthAgo, to );
    monthCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( monthCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( monthlyCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( monthCharts ) );
}


void
NetworkActivityWidget::showWeekCharts()
{
    d_func()->activeView = WeekChart;
    if ( !d_func()->weeklyChartsModel.isNull() )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->weeklyChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else
    {
        fetchWeekCharts();
    }
}

void
NetworkActivityWidget::showMonthCharts()
{
    d_func()->activeView = MonthChart;
    if ( !d_func()->monthlyChartsModel.isNull() )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->monthlyChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else
    {
        fetchMonthCharts();
    }
}

void
NetworkActivityWidget::showYearCharts()
{
    d_func()->activeView = YearChart;
    if ( !d_func()->yearlyChartsModel.isNull() )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->yearlyChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else
    {
        fetchYearCharts();
    }
}

void NetworkActivityWidget::showOverallCharts()
{
    d_func()->activeView = OverallChart;
    if ( !d_func()->overallChartsModel.isNull() )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->overallChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else
    {
        fetchOverallCharts();
    }
}
