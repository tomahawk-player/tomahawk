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

#include "NetworkActivityWidget.h"
#include "ui_NetworkActivityWidget.h"

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

NetworkActivityWidget::NetworkActivityWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::NetworkActivityWidget )
    , m_sortedProxy( 0 )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->stackLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->breadCrumbLeft->layout() );

    m_crumbModelLeft = new QStandardItemModel( this );
    m_sortedProxy = new QSortFilterProxyModel( this );
    m_sortedProxy->setDynamicSortFilter( true );
    m_sortedProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );

    ui->breadCrumbLeft->setRootIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::NetworkActivity, TomahawkUtils::Original ) );
    connect( ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged( QModelIndex ) ) );

    ui->tracksViewLeft->setHeaderHidden( true );
    ui->tracksViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    PlaylistChartItemDelegate* del = new PlaylistChartItemDelegate( ui->tracksViewLeft, ui->tracksViewLeft->proxyModel() );
    ui->tracksViewLeft->setItemDelegate( del );
    ui->tracksViewLeft->setUniformRowHeights( false );

    m_playlistInterface = ui->tracksViewLeft->playlistInterface();

    // Lets have a spinner until loaded
    ui->breadCrumbLeft->setVisible( false );
    m_spinner = new AnimatedSpinner( ui->tracksViewLeft );
    m_spinner->fadeIn();
}

NetworkActivityWidget::~NetworkActivityWidget()
{
}

Tomahawk::playlistinterface_ptr
NetworkActivityWidget::playlistInterface() const
{
    return m_playlistInterface;
}

bool
NetworkActivityWidget::isBeingPlayed() const
{
    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->tracksViewLeft->playlistInterface() )
        return true;

    return false;
}

bool
NetworkActivityWidget::jumpToCurrentTrack()
{
    if ( ui->tracksViewLeft->model() && ui->tracksViewLeft->jumpToCurrentTrack() )
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
    QSharedPointer<QMutexLocker> locker = QSharedPointer<QMutexLocker>( new QMutexLocker( &m_retrieveMutex ) );
    m_weeklyChartsModel = new PlaylistModel( ui->tracksViewLeft );
    m_weeklyChartsModel->startLoading();
    // Pipeline::instance()->resolve( tracks );
    m_weeklyChartsModel->appendTracks( tracks );
    m_weeklyChartsModel->finishLoading();

    checkDone( locker );
}

void
NetworkActivityWidget::monthlyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    QSharedPointer<QMutexLocker> locker = QSharedPointer<QMutexLocker>( new QMutexLocker( &m_retrieveMutex ) );
    m_monthlyChartsModel = new PlaylistModel( ui->tracksViewLeft );
    m_monthlyChartsModel->startLoading();
    // Pipeline::instance()->resolve( tracks );
    m_monthlyChartsModel->appendTracks( tracks );
    m_monthlyChartsModel->finishLoading();

    checkDone( locker );
}

void
NetworkActivityWidget::yearlyCharts( const QList<Tomahawk::track_ptr>& tracks )
{
    QSharedPointer<QMutexLocker> locker = QSharedPointer<QMutexLocker>( new QMutexLocker( &m_retrieveMutex ) );
    m_yearlyChartsModel = new PlaylistModel( ui->tracksViewLeft );
    m_yearlyChartsModel->startLoading();
    // Pipeline::instance()->resolve( tracks );
    m_yearlyChartsModel->appendTracks( tracks );
    m_yearlyChartsModel->finishLoading();

    checkDone( locker );
}

void
NetworkActivityWidget::leftCrumbIndexChanged( QModelIndex index )
{
    QStandardItem* item = m_crumbModelLeft->itemFromIndex( m_sortedProxy->mapToSource( index ) );
    if ( !item )
        return;
    if ( !item->data( Breadcrumb::ChartIdRole ).isValid() )
        return;

    const QString chartId = item->data( Breadcrumb::ChartIdRole ).toString();
    if ( chartId == NETWORKCHARTS_WEEK_CHARTS )
    {
        ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        ui->tracksViewLeft->setPlaylistModel( m_weeklyChartsModel );
        ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else if ( chartId == NETWORKCHARTS_MONTH_CHARTS )
    {
        ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        ui->tracksViewLeft->setPlaylistModel( m_monthlyChartsModel );
        ui->tracksViewLeft->proxyModel()->sort( -1 );
    }
    else if ( chartId == NETWORKCHARTS_YEAR_CHARTS )
    {
        ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
        ui->tracksViewLeft->setPlaylistModel( m_yearlyChartsModel );
        ui->tracksViewLeft->proxyModel()->sort( -1 );
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
NetworkActivityWidget::checkDone( QSharedPointer<QMutexLocker> )
{
    if ( !m_weeklyChartsModel.isNull() && !m_yearlyChartsModel.isNull() && !m_monthlyChartsModel.isNull() )
    {
        // All charts are loaded, do the remaining work UI work.

        // Build up breadcrumb
        QStandardItem* rootItem = m_crumbModelLeft->invisibleRootItem();
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
        m_sortedProxy->setSourceModel( m_crumbModelLeft );
        m_sortedProxy->sort( 0, Qt::AscendingOrder );
        ui->breadCrumbLeft->setModel( m_sortedProxy );

        m_spinner->fadeOut();
        ui->breadCrumbLeft->setVisible( true );
    }
}
