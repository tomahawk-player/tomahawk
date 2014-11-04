/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
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

#include "ChartsWidget.h"
#include "ui_ChartsWidget.h"

#include "MetaPlaylistInterface.h"
#include "TomahawkSettings.h"

#include "audio/AudioEngine.h"
#include "playlist/GridItemDelegate.h"
#include "widgets/ChartDataLoader.h"
#include "utils/AnimatedSpinner.h"
#include "utils/DpiScaler.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"

#include <QStandardItemModel>

using namespace Tomahawk;
using namespace Tomahawk::Widgets;

static QString s_chartsIdentifier = QString( "ChartsWidget" );


ChartsWidget::ChartsWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::ChartsWidget )
    , m_sortedProxy( 0 )
    , m_workerThread( 0 )
    , m_spinner( 0 )
    , m_loading( true )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->stackLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->breadCrumbLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );

    m_crumbModelLeft = new QStandardItemModel( this );
    m_sortedProxy = new QSortFilterProxyModel( this );
    m_sortedProxy->setDynamicSortFilter( true );
    m_sortedProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );

    ui->breadCrumbLeft->setRootIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::Charts, TomahawkUtils::Original ) );
    connect( ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged( QModelIndex ) ) );

    ui->artistsView->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 190 ) );
    ui->albumsView->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 190 ) );
    ui->tracksView->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 190 ) );
    ui->artistsView->delegate()->setShowPosition( true );
    ui->albumsView->delegate()->setShowPosition( true );
    ui->tracksView->delegate()->setShowPosition( true );

    m_workerThread = new QThread( this );
    m_workerThread->start();

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    // Read last viewed charts, to be used as defaults
    m_currentVIds = TomahawkSettings::instance()->lastChartIds();
    tDebug( LOGVERBOSE ) << "Reloading last chartIds:" << m_currentVIds;

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->tracksView->playlistInterface() );
    mpl->addChildInterface( ui->artistsView->playlistInterface() );
    mpl->addChildInterface( ui->albumsView->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( mpl );

    // Lets have a spinner until loaded
    ui->breadCrumbLeft->setVisible( false );
    ui->stackLeft->setCurrentIndex( 2 );
    m_spinner = new AnimatedSpinner( ui->albumsView );
    m_spinner->fadeIn();

    fetchData();
}


ChartsWidget::~ChartsWidget()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    // Write the settings
    TomahawkSettings::instance()->setLastChartIds( m_currentVIds );

    qDeleteAll( m_workers );
    m_workers.clear();
    m_workerThread->exit( 0 );

    delete m_spinner;
    delete ui;
}


Tomahawk::playlistinterface_ptr
ChartsWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
ChartsWidget::isBeingPlayed() const
{
    return playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() );
}


bool
ChartsWidget::jumpToCurrentTrack()
{
    if ( ui->artistsView->model() && ui->artistsView->jumpToCurrentTrack() )
        return true;

    if ( ui->tracksView->model() && ui->tracksView->jumpToCurrentTrack() )
        return true;

    if ( ui->albumsView->model() && ui->albumsView->jumpToCurrentTrack() )
        return true;

    return false;
}


void
ChartsWidget::fetchData()
{
    Tomahawk::InfoSystem::InfoStringHash criteria;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_chartsIdentifier;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
    requestData.type = Tomahawk::InfoSystem::InfoChartCapabilities;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "requested InfoChartCapabilities";
}


void
ChartsWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_chartsIdentifier )
        return;

    if ( output.isNull() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Info came back empty";
        return;
    }

    if ( !output.canConvert< QVariantMap >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not parse output into a map";
        return;
    }

    QVariantMap returnedData = output.toMap();
    switch ( requestData.type )
    {
        case InfoSystem::InfoChartCapabilities:
        {
            setViewData( returnedData );
        }
        case InfoSystem::InfoChart:
        {
            if ( returnedData.contains( "chart_error" ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Info came back with error!";

                Tomahawk::InfoSystem::InfoStringHash criteria;
                criteria.insert( "chart_refetch", returnedData[ "chart_source" ].value< QString >() );

                Tomahawk::InfoSystem::InfoRequestData requestData;
                requestData.caller = s_chartsIdentifier;
                requestData.customData = QVariantMap();
                requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
                requestData.type = Tomahawk::InfoSystem::InfoChartCapabilities;
                requestData.timeoutMillis = 20000;
                requestData.allSources = false;
                Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "re-requesting InfoChartCapabilities";
                break;
            }

            if ( !returnedData.contains( "type" ) )
                break;

            const QString type = returnedData[ "type" ].toString();
            if ( !returnedData.contains( type ) )
                break;

            const QString chartId = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >().value( "chart_id" );

            m_queuedFetches.remove( chartId );

            ChartDataLoader* loader = new ChartDataLoader();
            loader->setProperty( "chartid", chartId );
            loader->moveToThread( m_workerThread );

            if ( type == "artists" )
            {
                loader->setType( ChartDataLoader::Artist );
                loader->setData( returnedData[ "artists" ].value< QStringList >() );

                connect( loader, SIGNAL( artists( Tomahawk::ChartDataLoader*, QList< Tomahawk::artist_ptr > ) ), SLOT( chartArtistsLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::artist_ptr > ) ) );

                PlayableModel* artistsModel = new PlayableModel( ui->artistsView );
                artistsModel->startLoading();

                m_artistModels[ chartId ] = artistsModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewArtists( artistsModel );
            }
            else if ( type == "albums" )
            {
                loader->setType( ChartDataLoader::Album );
                loader->setData( returnedData[ "albums" ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >() );

                connect( loader, SIGNAL( albums( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ), SLOT( chartAlbumsLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ) );

                PlayableModel* albumModel = new PlayableModel( ui->albumsView );
                albumModel->startLoading();

                m_albumModels[ chartId ] = albumModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewAlbums( albumModel );
            }
            else if ( type == "tracks" )
            {
                loader->setType( ChartDataLoader::Track );
                loader->setData( returnedData[ "tracks" ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >() );

                connect( loader, SIGNAL( tracks( Tomahawk::ChartDataLoader*, QList< Tomahawk::query_ptr > ) ), SLOT( chartTracksLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::query_ptr > ) ) );

                PlayableModel* trackModel = new PlayableModel( ui->tracksView );
                trackModel->startLoading();

                m_trackModels[ chartId ] = trackModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewTracks( trackModel );

            }

            QMetaObject::invokeMethod( loader, "go", Qt::QueuedConnection );
            break;
        }

        default:
            return;
    }
}


void
ChartsWidget::setViewData( const QVariantMap& data )
{
    QStandardItem* rootItem = m_crumbModelLeft->invisibleRootItem();
    QVariantMap returnedData = data;

    // We need to take this from data
    QString defaultSource = returnedData.take( "defaultSource" ).toString();
    QVariantMap defaults = returnedData.take( "defaults" ).toMap();
    // Here, we dont want current sessions last view, but rather what was current on previus quit
    QString lastSeen = TomahawkSettings::instance()->lastChartIds().value( "lastseen" ).toString();

    if ( !lastSeen.isEmpty() )
        defaultSource = lastSeen;

    // Merge defaults with current defaults, split the value in to a list
    foreach ( const QString& key, m_currentVIds.keys() )
        defaults[ key ] = m_currentVIds.value( key ).toString().split( "/" );

    foreach ( const QString& label, returnedData.keys() )
    {
        QStandardItem* childItem = parseNode( rootItem, label, returnedData[ label ] );
        const QString id = label.toLower();

        if ( id == defaultSource.toLower() )
        {
            qDebug() << "Setting source as default" << id;
            childItem->setData( true, Breadcrumb::DefaultRole );
        }
        if ( defaults.contains( id ) )
        {
            QStringList defaultIndices = defaults[ id ].toStringList();
            QStandardItem* cur = childItem;

            foreach ( const QString& index, defaultIndices )
            {
                // Go through the children of the current item, marking the default one as default
                for ( int k = 0; k < cur->rowCount(); k++ )
                {
                    if ( cur->child( k, 0 )->text().toLower() == index.toLower() )
                    {
                        qDebug() << "Setting child as default" << index;
                        cur = cur->child( k, 0 ); // this is the default, drill down into the default to pick the next default
                        cur->setData( true, Breadcrumb::DefaultRole );
                        break;
                    }
                }
            }
        }
        rootItem->appendRow( childItem );
    }
}


void
ChartsWidget::infoSystemFinished( QString target )
{
    if ( m_loading )
    {
        if ( target != s_chartsIdentifier )
        {
            return;
        }

        m_sortedProxy->setSourceModel( m_crumbModelLeft );
        m_sortedProxy->sort( 0, Qt::AscendingOrder );
        ui->breadCrumbLeft->setModel( m_sortedProxy );

        m_spinner->fadeOut();
        ui->breadCrumbLeft->setVisible( true );
        m_loading = false;
    }
}


void
ChartsWidget::leftCrumbIndexChanged( QModelIndex index )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "left crumb changed" << index.data();
    QStandardItem* item = m_crumbModelLeft->itemFromIndex( m_sortedProxy->mapToSource( index ) );
    if ( !item )
        return;
    if ( !item->data( Breadcrumb::ChartIdRole ).isValid() )
        return;

    // Build current views as default. Will be used on next restart
    QStringList curr;
    curr.append( index.data().toString().toLower() ); // This chartname

    QList<QModelIndex> indexes;

    while ( index.parent().isValid() )
    {
        indexes.prepend(index);
        index = index.parent();
        curr.prepend( index.data().toString().toLower() );
    }
    const QString chartId = item->data( Breadcrumb::ChartIdRole ).toString();
    const qlonglong chartExpires = item->data( Breadcrumb::ChartExpireRole ).toLongLong();
    const QString chartSource = curr.takeFirst().toLower();
    curr.append( chartSource );
    curr.append( chartId );

    // Write the current view
    m_currentVIds[ chartSource ] = curr.join( "/" ); // Instead of keeping an array, join and split later
    m_currentVIds[ "lastseen" ] = chartSource; // We keep a record of last seen

    if ( m_artistModels.contains( chartId ) )
    {
        setLeftViewArtists( m_artistModels[ chartId ] );
        return;
    }
    else if ( m_albumModels.contains( chartId ) )
    {
        setLeftViewAlbums( m_albumModels[ chartId ] );
        return;
    }
    else if ( m_trackModels.contains( chartId ) )
    {
        setLeftViewTracks( m_trackModels[ chartId ] );
        return;
    }

    if ( m_queuedFetches.contains( chartId ) )
    {
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria.insert( "chart_id", chartId );
    criteria.insert( "chart_expires", QString::number( chartExpires ) );
    /// Remember to lower the source!
    criteria.insert( "chart_source",  index.data().toString().toLower() );

    Tomahawk::InfoSystem::InfoRequestData requestData;
    QVariantMap customData;
    customData.insert( "whatshot_side", "left" );
    requestData.caller = s_chartsIdentifier;
    requestData.customData = customData;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
    requestData.type = Tomahawk::InfoSystem::InfoChart;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;

    qDebug() << "Making infosystem request for chart of type:" << chartId;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    m_queuedFetches.insert( chartId );
    m_queueItemToShow = chartId;
}


void
ChartsWidget::changeEvent( QEvent* e )
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
ChartsWidget::parseNode( QStandardItem* parentItem, const QString& label, const QVariant& data )
{
    Q_UNUSED( parentItem );
//     tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "parsing" << label;

    QStandardItem* sourceItem = new QStandardItem( label );

    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
        QList< Tomahawk::InfoSystem::InfoStringHash > charts = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();

        foreach ( Tomahawk::InfoSystem::InfoStringHash chart, charts )
        {
            QStandardItem* childItem= new QStandardItem( chart[ "label" ] );
            childItem->setData( chart[ "id" ], Breadcrumb::ChartIdRole );
            childItem->setData( chart[ "expires" ], Breadcrumb::ChartExpireRole );

            if ( m_currentVIds.contains( chart.value( "id" ).toLower() ) )
            {
                 childItem->setData( true, Breadcrumb::DefaultRole );
            }
            else if ( chart.value( "default", "" ) == "true" )
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
            QStandardItem* childItem  = parseNode( sourceItem, childLabel, dataMap[ childLabel ] );
            sourceItem->appendRow( childItem );
        }
    }
    else if ( data.canConvert<QVariantList>() )
    {
        QVariantList dataList = data.toList();

        foreach ( const QVariant value, dataList )
        {
            QStandardItem* childItem = new QStandardItem( value.toString() );
            sourceItem->appendRow( childItem );
        }
    }
    else
    {
        QStandardItem* childItem = new QStandardItem( data.toString() );
        sourceItem->appendRow( childItem );
    }
    return sourceItem;
}


void
ChartsWidget::setLeftViewAlbums( PlayableModel* model )
{
    ui->albumsView->setPlayableModel( model );
    ui->albumsView->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsView->setTreeModel
    ui->stackLeft->setCurrentIndex( 2 );
}


void
ChartsWidget::setLeftViewArtists( PlayableModel* model )
{
    ui->artistsView->setPlayableModel( model );
    ui->artistsView->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsView->setTreeModel
    ui->stackLeft->setCurrentIndex( 1 );
}


void
ChartsWidget::setLeftViewTracks( PlayableModel* model )
{
    ui->tracksView->setPlayableModel( model );
    ui->tracksView->proxyModel()->sort( -1 );
    ui->stackLeft->setCurrentIndex( 0 );
}


void
ChartsWidget::chartArtistsLoaded( ChartDataLoader* loader, const QList< artist_ptr >& artists )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_artistModels.contains( chartId ) );

    if ( m_artistModels.contains( chartId ) )
    {
        m_artistModels[ chartId ]->appendArtists( artists );
    }

    m_workers.remove( loader );
    loader->deleteLater();
}


void
ChartsWidget::chartTracksLoaded( ChartDataLoader* loader, const QList< query_ptr >& tracks )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_trackModels.contains( chartId ) );

    if ( m_trackModels.contains( chartId ) )
    {
        m_trackModels[ chartId ]->appendQueries( tracks );
    }

    m_workers.remove( loader );
    loader->deleteLater();
}


void
ChartsWidget::chartAlbumsLoaded( ChartDataLoader* loader, const QList< album_ptr >& albums )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_albumModels.contains( chartId ) );

    if ( m_albumModels.contains( chartId ) )
    {
        m_albumModels[ chartId ]->appendAlbums( albums );
    }

    m_workers.remove( loader );
    loader->deleteLater();
}


ChartsPage::ChartsPage( QWidget* parent )
{
    Q_UNUSED( parent )
}


ChartsPage::~ChartsPage()
{
}

Q_EXPORT_PLUGIN2( ViewPagePlugin, ChartsPage )
