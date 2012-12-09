/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Casey Link <unnamedrambler@gmail.com>
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "NewReleasesWidget.h"
#include "ui_NewReleasesWidget.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "RecentPlaylistsModel.h"
#include "ChartDataLoader.h"

#include "audio/AudioEngine.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "playlist/PlaylistChartItemDelegate.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "Pipeline.h"

#include <QPainter>
#include <QStandardItemModel>
#include <QStandardItem>


#define HISTORY_TRACK_ITEMS 25
#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_RESOLVING_TIMEOUT 2500

using namespace Tomahawk;

static QString s_newReleasesIdentifier = QString( "NewReleasesWidget" );


NewReleasesWidget::NewReleasesWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::NewReleasesWidget )
    , m_sortedProxy( 0 )
    , m_workerThread( 0 )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2 );
    TomahawkUtils::unmarginLayout( ui->breadCrumbLeft->layout() );

    m_crumbModelLeft = new QStandardItemModel( this );
    m_sortedProxy = new QSortFilterProxyModel( this );
    m_sortedProxy->setDynamicSortFilter( true );
    m_sortedProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );

    ui->breadCrumbLeft->setRootIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::NewReleases, TomahawkUtils::Original ) );

    connect( ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged(QModelIndex) ) );

    //m_playlistInterface = Tomahawk::playlistinterface_ptr( new ChartsPlaylistInterface( this ) );

    m_workerThread = new QThread( this );
    m_workerThread->start();

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
}


NewReleasesWidget::~NewReleasesWidget()
{
    qDeleteAll( m_workers );
    m_workers.clear();
    m_workerThread->exit(0);
    m_playlistInterface.clear();
    delete ui;
}


Tomahawk::playlistinterface_ptr
NewReleasesWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
NewReleasesWidget::isBeingPlayed() const
{
    return false;
}


bool
NewReleasesWidget::jumpToCurrentTrack()
{
    return false;
}


void
NewReleasesWidget::fetchData()
{
    Tomahawk::InfoSystem::InfoStringHash artistInfo;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_newReleasesIdentifier;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
    requestData.type = Tomahawk::InfoSystem::InfoNewReleaseCapabilities;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Requested InfoNewReleaseCapabilities";
}


void
NewReleasesWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_newReleasesIdentifier )
        return;

    if ( !output.canConvert< QVariantMap >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not parse output";
        return;
    }

    QVariantMap returnedData = output.toMap();
    switch ( requestData.type )
    {
        case InfoSystem::InfoNewReleaseCapabilities:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got InfoNewReleaseCapabilities";
            QStandardItem *rootItem= m_crumbModelLeft->invisibleRootItem();

            foreach ( const QString label, returnedData.keys() )
            {
                QStandardItem *childItem = parseNode( rootItem, label, returnedData[label] );
                rootItem->appendRow(childItem);
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "NewReleases:" << label;
            }

            m_sortedProxy->setSourceModel( m_crumbModelLeft );
            m_sortedProxy->sort( 0, Qt::AscendingOrder );
            ui->breadCrumbLeft->setModel( m_sortedProxy );
            break;
        }

        case InfoSystem::InfoNewRelease:
        {
            if( !returnedData.contains("type") )
                break;
            const QString type = returnedData["type"].toString();
            if( !returnedData.contains(type) )
                break;

            const QString releaseId = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >().value( "nr_id" );

            m_queuedFetches.remove( releaseId );

            ChartDataLoader* loader = new ChartDataLoader();
            loader->setProperty( "nrid", releaseId );
            loader->moveToThread( m_workerThread );

            if ( type == "albums" )
            {
                loader->setType( ChartDataLoader::Album );
                loader->setData( returnedData[ "albums" ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >() );

                connect( loader, SIGNAL( albums( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ), this, SLOT( newReleasesLoaded( Tomahawk::ChartDataLoader*, QList<Tomahawk::album_ptr> ) ) );

                PlayableModel* albumModel = new PlayableModel( ui->albumsView );

                m_albumModels[ releaseId ] = albumModel;

                if ( m_queueItemToShow == releaseId )
                    setLeftViewAlbums( albumModel );
            }
            else
            {
                // intentionally unhandled
            }

            QMetaObject::invokeMethod( loader, "go", Qt::QueuedConnection );

            break;
        }

        default:
            return;
    }
}


void
NewReleasesWidget::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
NewReleasesWidget::leftCrumbIndexChanged( QModelIndex index )
{
    tDebug( LOGVERBOSE ) << "NewReleases:: left crumb changed" << index.data();
    QStandardItem* item = m_crumbModelLeft->itemFromIndex( m_sortedProxy->mapToSource( index ) );
    if( !item )
        return;
    if( !item->data( Breadcrumb::ChartIdRole ).isValid() )
        return;


    QList<QModelIndex> indexes;
    while ( index.parent().isValid() )
    {
        indexes.prepend(index);
        index = index.parent();
    }


    const QString nrId = item->data( Breadcrumb::ChartIdRole ).toString();
    const qlonglong nrExpires = item->data( Breadcrumb::ChartExpireRole ).toLongLong();

    if ( m_albumModels.contains( nrId ) )
    {
        setLeftViewAlbums( m_albumModels[ nrId ] );
        return;
    }

    if ( m_queuedFetches.contains( nrId ) )
    {
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria.insert( "nr_id", nrId );
    criteria.insert( "nr_expires", QString::number(nrExpires) );
    /// Remember to lower the source!
    criteria.insert( "nr_source",  index.data().toString().toLower() );

    Tomahawk::InfoSystem::InfoRequestData requestData;
    QVariantMap customData;
    customData.insert( "newrelease_side", "left" );
    requestData.caller = s_newReleasesIdentifier;
    requestData.customData = customData;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
    requestData.type = Tomahawk::InfoSystem::InfoNewRelease;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;

    tDebug( LOGVERBOSE ) << "Making infosystem request for chart of type:" << nrId;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    m_queuedFetches.insert( nrId );
    m_queueItemToShow = nrId;
}


void
NewReleasesWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


QStandardItem*
NewReleasesWidget::parseNode( QStandardItem* parentItem, const QString &label, const QVariant &data )
{
    Q_UNUSED( parentItem );
//     tDebug( LOGVERBOSE ) << "NewReleases:: parsing " << label;

    QStandardItem *sourceItem = new QStandardItem(label);

    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
        QList< Tomahawk::InfoSystem::InfoStringHash > charts = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
        foreach ( Tomahawk::InfoSystem::InfoStringHash chart, charts )
        {
            QStandardItem *childItem= new QStandardItem( chart[ "label" ] );
            childItem->setData( chart[ "id" ], Breadcrumb::ChartIdRole );
            childItem->setData( chart[ "expires" ], Breadcrumb::ChartExpireRole );
            if ( chart.value( "default", "" ) == "true")
            {
                childItem->setData( true, Breadcrumb::DefaultRole );
            }
            sourceItem->appendRow( childItem );
        }
    }
    else if ( data.canConvert<QVariantMap>() )
    {
        QVariantMap dataMap = data.toMap();
        foreach ( const QString childLabel,dataMap.keys() )
        {
            QStandardItem *childItem  = parseNode( sourceItem, childLabel, dataMap[childLabel] );
            sourceItem->appendRow( childItem );
        }
    }
    else if ( data.canConvert<QVariantList>() )
    {
        QVariantList dataList = data.toList();

        foreach ( const QVariant value, dataList )
        {
            QStandardItem *childItem= new QStandardItem(value.toString());
            sourceItem->appendRow(childItem);
        }
    }
    else
    {
        QStandardItem *childItem= new QStandardItem( data.toString() );
        sourceItem->appendRow( childItem );
    }
    return sourceItem;
}


void
NewReleasesWidget::setLeftViewAlbums( PlayableModel* model )
{
    ui->albumsView->setPlayableModel( model );
    ui->albumsView->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
}


void
NewReleasesWidget::newReleasesLoaded( ChartDataLoader* loader, const QList< album_ptr >& albums )
{
    QString chartId = loader->property( "nrid" ).toString();
    Q_ASSERT( m_albumModels.contains( chartId ) );

    if ( m_albumModels.contains( chartId ) )
        m_albumModels[ chartId ]->appendAlbums( albums );

    m_workers.remove( loader );
    loader->deleteLater();
}
