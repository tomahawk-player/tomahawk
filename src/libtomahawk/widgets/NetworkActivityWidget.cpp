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
#include "utils/TomahawkUtilsGui.h"

#include <QDateTime>
#include <QStandardItemModel>
#include <QtConcurrentRun>

#define NETWORKCHARTS_NUM_TRACKS 100
#define NETWORKCHARTS_WEEK_CHARTS "week"
#define NETWORKCHARTS_MONTH_CHARTS "month"
#define NETWORKCHARTS_YEAR_CHARTS "year"

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

    // Lets have a spinner until loaded
    d_func()->ui->breadCrumbLeft->setVisible( false );
    d_func()->spinner = new AnimatedSpinner( d_func()->ui->tracksViewLeft );
    d_func()->spinner->fadeIn();
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
NetworkActivityWidget::fetchData()
{
    // Do not block the UI thread
    QtConcurrent::run( this, &NetworkActivityWidget::actualFetchData );
}


void
NetworkActivityWidget::weeklyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    d_func()->weeklyChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->weeklyChartsModel->startLoading();
    // Pipeline::instance()->resolve( tracks );
    d_func()->weeklyChartsModel->appendTracks( tracks );
    d_func()->weeklyChartsModel->finishLoading();

    checkDone();
}


void
NetworkActivityWidget::monthlyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    d_func()->monthlyChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->monthlyChartsModel->startLoading();
    // Pipeline::instance()->resolve( tracks );
    d_func()->monthlyChartsModel->appendTracks( tracks );
    d_func()->monthlyChartsModel->finishLoading();

    checkDone();
}


void
NetworkActivityWidget::yearlyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    d_func()->yearlyChartsModel = new PlaylistModel( d_func()->ui->tracksViewLeft );
    d_func()->yearlyChartsModel->startLoading();
    // Pipeline::instance()->resolve( tracks );
    d_func()->yearlyChartsModel->appendTracks( tracks );
    d_func()->yearlyChartsModel->finishLoading();

    checkDone();
}


void
NetworkActivityWidget::leftCrumbIndexChanged( const QModelIndex& index )
{
    QStandardItem* item = d_func()->crumbModelLeft->itemFromIndex( d_func()->sortedProxy->mapToSource( index ) );
    if ( !item )
        return;
    if ( !item->data( Breadcrumb::ChartIdRole ).isValid() )
        return;

    const QString chartId = item->data( Breadcrumb::ChartIdRole ).toString();
    if ( chartId == NETWORKCHARTS_WEEK_CHARTS )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->weeklyChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else if ( chartId == NETWORKCHARTS_MONTH_CHARTS )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->monthlyChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else if ( chartId == NETWORKCHARTS_YEAR_CHARTS )
    {
        d_func()->ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        d_func()->ui->tracksViewLeft->setPlaylistModel( d_func()->yearlyChartsModel );
        d_func()->ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
}


void
NetworkActivityWidget::actualFetchData()
{
    QDateTime to = QDateTime::currentDateTime();

    // Weekly charts
    QDateTime weekAgo = to.addDays( -7 );
    DatabaseCommand_NetworkCharts* weekCharts = new DatabaseCommand_NetworkCharts( weekAgo, to );
    weekCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( weekCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( weeklyCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( weekCharts ) );

    // Monthly charts
    QDateTime monthAgo = to.addMonths( -1 );
    DatabaseCommand_NetworkCharts* monthCharts = new DatabaseCommand_NetworkCharts( monthAgo, to );
    monthCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( monthCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( monthlyCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( monthCharts ) );

    // Yearly charts
    QDateTime yearAgo = to.addYears( -1 );
    DatabaseCommand_NetworkCharts* yearCharts = new DatabaseCommand_NetworkCharts( yearAgo, to );
    yearCharts->setLimit( NETWORKCHARTS_NUM_TRACKS );
    connect( yearCharts, SIGNAL( done( QList<Tomahawk::track_ptr> ) ), SLOT( yearlyCharts( QList<Tomahawk::track_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( yearCharts ) );
}


void
NetworkActivityWidget::checkDone()
{
    if ( !d_func()->weeklyChartsModel.isNull() && !d_func()->yearlyChartsModel.isNull() && !d_func()->monthlyChartsModel.isNull() )
    {
        // All charts are loaded, do the remaining work UI work.

        // Build up breadcrumb
        QStandardItem* rootItem = d_func()->crumbModelLeft->invisibleRootItem();
        QStandardItem* chartItem = new QStandardItem( tr( "Charts" ) );
        rootItem->appendRow( chartItem );
        QStandardItem* weekItem = new QStandardItem( tr( "Last Week" ) );
        weekItem->setData( NETWORKCHARTS_WEEK_CHARTS, Breadcrumb::ChartIdRole );
        chartItem->appendRow( weekItem );
        QStandardItem* monthItem = new QStandardItem( tr( "Last Month" ) );
        monthItem->setData( NETWORKCHARTS_MONTH_CHARTS, Breadcrumb::ChartIdRole );
        chartItem->appendRow( monthItem );
        QStandardItem* yearItem = new QStandardItem( tr( "Last Year" ) );
        yearItem->setData( NETWORKCHARTS_YEAR_CHARTS, Breadcrumb::ChartIdRole );
        chartItem->appendRow( yearItem );
        d_func()->sortedProxy->setSourceModel( d_func()->crumbModelLeft );
        d_func()->sortedProxy->sort( 0, Qt::AscendingOrder );
        d_func()->ui->breadCrumbLeft->setModel( d_func()->sortedProxy );

        d_func()->spinner->fadeOut();
        d_func()->ui->breadCrumbLeft->setVisible( true );
    }
}
