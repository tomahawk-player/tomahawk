/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
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

#include "WhatsHotWidget.h"
#include "ui_WhatsHotWidget.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "RecentPlaylistsModel.h"
#include "ChartDataLoader.h"
#include "MetaPlaylistInterface.h"

#include "audio/AudioEngine.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlayableModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "playlist/PlaylistChartItemDelegate.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "Pipeline.h"
#include "utils/AnimatedSpinner.h"

#include <QPainter>
#include <QStandardItemModel>
#include <QStandardItem>


#define HISTORY_TRACK_ITEMS 25
#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_RESOLVING_TIMEOUT 2500

using namespace Tomahawk;

static QString s_whatsHotIdentifier = QString( "WhatsHotWidget" );


WhatsHotWidget::WhatsHotWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WhatsHotWidget )
    , m_sortedProxy( 0 )
    , m_workerThread( 0 )
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

    ui->tracksViewLeft->setHeaderHidden( true );
    ui->tracksViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    PlaylistChartItemDelegate* del = new PlaylistChartItemDelegate( ui->tracksViewLeft, ui->tracksViewLeft->proxyModel() );
    ui->tracksViewLeft->setItemDelegate( del );
    ui->tracksViewLeft->setUniformRowHeights( false );

    TreeProxyModel* artistsProxy = new TreeProxyModel( ui->artistsViewLeft );
    artistsProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
    artistsProxy->setDynamicSortFilter( true );

    ui->artistsViewLeft->setProxyModel( artistsProxy );
    ui->artistsViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->artistsViewLeft->header()->setVisible( true );

    m_workerThread = new QThread( this );
    m_workerThread->start();

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    // Read last viewed charts, to be used as defaults
    m_currentVIds = TomahawkSettings::instance()->lastChartIds();
    qDebug() << "Got last chartIds:" << m_currentVIds;

    // TracksView is first shown, show spinner on that
    // After fadeOut, charts are loaded
    m_loadingSpinner =  new AnimatedSpinner( ui->tracksViewLeft );
    m_loadingSpinner->fadeIn();

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->tracksViewLeft->playlistInterface() );
    mpl->addChildInterface( ui->artistsViewLeft->playlistInterface() );
    mpl->addChildInterface( ui->albumsView->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( mpl );
}


WhatsHotWidget::~WhatsHotWidget()
{
    qDebug() << "Deleting whatshot";
    // Write the settings
    qDebug() << "Writing chartIds to settings:" << m_currentVIds;
    TomahawkSettings::instance()->setLastChartIds( m_currentVIds );
    qDeleteAll( m_workers );
    m_workers.clear();
    m_workerThread->exit( 0 );
    m_playlistInterface.clear();
    delete ui;
}


Tomahawk::playlistinterface_ptr
WhatsHotWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
WhatsHotWidget::isBeingPlayed() const
{
    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->artistsViewLeft->playlistInterface() )
        return true;

    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->tracksViewLeft->playlistInterface() )
        return true;

    if ( ui->albumsView->isBeingPlayed() )
        return true;

    return false;
}


bool
WhatsHotWidget::jumpToCurrentTrack()
{
    if ( ui->artistsViewLeft->model() && ui->artistsViewLeft->jumpToCurrentTrack() )
        return true;

    if ( ui->tracksViewLeft->model() && ui->tracksViewLeft->jumpToCurrentTrack() )
        return true;

    if ( ui->albumsView->model() && ui->albumsView->jumpToCurrentTrack() )
        return true;

    return false;
}


void
WhatsHotWidget::fetchData()
{
    Tomahawk::InfoSystem::InfoStringHash criteria;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_whatsHotIdentifier;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
    requestData.type = Tomahawk::InfoSystem::InfoChartCapabilities;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    tDebug( LOGVERBOSE ) << "WhatsHot: requested InfoChartCapabilities";
}


void
WhatsHotWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_whatsHotIdentifier )
        return;

    if ( output.isNull() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Info came back empty";
        return;
    }

    if ( !output.canConvert< QVariantMap >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "WhatsHot: Could not parse output into a map";
        return;
    }

    QVariantMap returnedData = output.toMap();
    switch ( requestData.type )
    {
        case InfoSystem::InfoChartCapabilities:
        {
            QStandardItem *rootItem= m_crumbModelLeft->invisibleRootItem();
            QVariantMap defaults;
            if ( returnedData.contains( "defaults" ) )
                defaults = returnedData.take( "defaults" ).toMap();

            // We need to take this from data
            QString defaultSource = returnedData.take( "defaultSource" ).toString();
            // Here, we dont want current sessions last view, but rather what was current on previus quit
            QString lastSeen = TomahawkSettings::instance()->lastChartIds().value( "lastseen" ).toString();
            if( !lastSeen.isEmpty() )
                defaultSource = lastSeen;

            // Merge defaults with current defaults, split the value in to a list
            foreach( const QString&key, m_currentVIds.keys() )
                defaults[ key ] = m_currentVIds.value( key ).toString().split( "/" );
            qDebug() << "Defaults after merge" << defaults;
            foreach ( const QString label, returnedData.keys() )
            {
                QStandardItem *childItem = parseNode( rootItem, label, returnedData[ label ] );
                rootItem->appendRow( childItem );
            }

            // Set the default source
            // Set the default chart for each source
            if( !defaults.empty() )
            {
                for ( int i = 0; i < rootItem->rowCount(); i++ )
                {
                    QStandardItem* source = rootItem->child( i, 0 );
                    if ( defaultSource.toLower() == source->text().toLower() )
                    {
                        source->setData( true, Breadcrumb::DefaultRole );
                    }

                    if ( defaults.contains( source->text().toLower() ) )
                    {
                        QStringList defaultIndices = defaults[ source->text().toLower() ].toStringList();
                        QStandardItem* cur = source;

                        foreach( const QString& index, defaultIndices )
                        {
                            // Go through the children of the current item, marking the default one as default
                            for ( int k = 0; k < cur->rowCount(); k++ )
                            {
                                if ( cur->child( k, 0 )->text().toLower() == index.toLower() )
                                {
                                    cur = cur->child( k, 0 ); // this is the default, drill down into the default to pick the next default
                                    cur->setData( true, Breadcrumb::DefaultRole );
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            m_sortedProxy->setSourceModel( m_crumbModelLeft );
            m_sortedProxy->sort( 0, Qt::AscendingOrder );
            ui->breadCrumbLeft->setModel( m_sortedProxy );
            break;
        }

        case InfoSystem::InfoChart:
        {

            if( returnedData.contains( "chart_error") )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Info came back with error!!";

                Tomahawk::InfoSystem::InfoStringHash criteria;
                criteria.insert( "chart_refetch", returnedData[ "chart_source" ].value< QString >() );

                Tomahawk::InfoSystem::InfoRequestData requestData;
                requestData.caller = s_whatsHotIdentifier;
                requestData.customData = QVariantMap();
                requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
                requestData.type = Tomahawk::InfoSystem::InfoChartCapabilities;
                requestData.timeoutMillis = 20000;
                requestData.allSources = false;
                Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

                tDebug( LOGVERBOSE ) << "WhatsHot: re-requesting InfoChartCapabilities";
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

                connect( loader, SIGNAL( artists( Tomahawk::ChartDataLoader*, QList< Tomahawk::artist_ptr > ) ), this, SLOT( chartArtistsLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::artist_ptr > ) ) );

                TreeModel* artistsModel = new TreeModel( ui->artistsViewLeft );
                artistsModel->setMode( InfoSystemMode );
                artistsModel->startLoading();

                m_artistModels[ chartId ] = artistsModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewArtists( artistsModel );
            }
            else if ( type == "albums" )
            {
                loader->setType( ChartDataLoader::Album );
                loader->setData( returnedData[ "albums" ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >() );

                connect( loader, SIGNAL( albums( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ), this, SLOT( chartAlbumsLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ) );

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

                connect( loader, SIGNAL( tracks( Tomahawk::ChartDataLoader*, QList< Tomahawk::query_ptr > ) ), this, SLOT( chartTracksLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::query_ptr > ) ) );

                PlaylistModel* trackModel = new PlaylistModel( ui->tracksViewLeft );
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
WhatsHotWidget::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
    m_loadingSpinner->fadeOut();
}


void
WhatsHotWidget::leftCrumbIndexChanged( QModelIndex index )
{
    tDebug( LOGVERBOSE ) << "WhatsHot: left crumb changed" << index.data();
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
    requestData.caller = s_whatsHotIdentifier;
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
WhatsHotWidget::changeEvent( QEvent* e )
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
WhatsHotWidget::parseNode( QStandardItem* parentItem, const QString &label, const QVariant &data )
{
    Q_UNUSED( parentItem );
//     tDebug( LOGVERBOSE ) << "WhatsHot: parsing " << label;

    QStandardItem *sourceItem = new QStandardItem( label );

    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
        QList< Tomahawk::InfoSystem::InfoStringHash > charts = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();

        foreach ( Tomahawk::InfoSystem::InfoStringHash chart, charts )
        {
            QStandardItem *childItem= new QStandardItem( chart[ "label" ] );
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
            QStandardItem *childItem  = parseNode( sourceItem, childLabel, dataMap[ childLabel ] );
            sourceItem->appendRow( childItem );
        }
    }
    else if ( data.canConvert<QVariantList>() )
    {
        QVariantList dataList = data.toList();

        foreach ( const QVariant value, dataList )
        {
            QStandardItem *childItem = new QStandardItem( value.toString() );
            sourceItem->appendRow( childItem );
        }
    }
    else
    {
        QStandardItem *childItem = new QStandardItem( data.toString() );
        sourceItem->appendRow( childItem );
    }
    return sourceItem;
}


void
WhatsHotWidget::setLeftViewAlbums( PlayableModel* model )
{
    ui->albumsView->setPlayableModel( model );
    ui->albumsView->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
    ui->stackLeft->setCurrentIndex( 2 );
}


void
WhatsHotWidget::setLeftViewArtists( TreeModel* model )
{
    ui->artistsViewLeft->proxyModel()->setStyle( PlayableProxyModel::Collection );
    ui->artistsViewLeft->setTreeModel( model );
    ui->artistsViewLeft->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
    ui->stackLeft->setCurrentIndex( 1 );
}


void
WhatsHotWidget::setLeftViewTracks( PlaylistModel* model )
{
    ui->tracksViewLeft->proxyModel()->setStyle( PlayableProxyModel::Large );
    ui->tracksViewLeft->setPlaylistModel( model );
    ui->tracksViewLeft->proxyModel()->sort( -1 );
    ui->stackLeft->setCurrentIndex( 0 );
}


void
WhatsHotWidget::chartArtistsLoaded( ChartDataLoader* loader, const QList< artist_ptr >& artists )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_artistModels.contains( chartId ) );

    if ( m_artistModels.contains( chartId ) )
    {
        foreach( const artist_ptr& artist, artists )
        {
            m_artistModels[ chartId ]->addArtists( artist );
            m_artistModels[ chartId ]->finishLoading();
        }
    }

    m_workers.remove( loader );
    loader->deleteLater();
}


void
WhatsHotWidget::chartTracksLoaded( ChartDataLoader* loader, const QList< query_ptr >& tracks )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_trackModels.contains( chartId ) );

    if ( m_trackModels.contains( chartId ) )
    {
        Pipeline::instance()->resolve( tracks );
        m_trackModels[ chartId ]->appendQueries( tracks );
        m_trackModels[ chartId ]->finishLoading();
    }

    m_workers.remove( loader );
    loader->deleteLater();
}


void
WhatsHotWidget::chartAlbumsLoaded( ChartDataLoader* loader, const QList< album_ptr >& albums )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_albumModels.contains( chartId ) );

    if ( m_albumModels.contains( chartId ) )
    {
        m_albumModels[ chartId ]->appendAlbums( albums );
        m_albumModels[ chartId ]->finishLoading();
    }

    m_workers.remove( loader );
    loader->deleteLater();
}
