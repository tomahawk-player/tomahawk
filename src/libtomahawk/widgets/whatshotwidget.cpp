/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "whatshotwidget.h"
#include "ui_whatshotwidget.h"

#include <QPainter>
#include <QStandardItemModel>
#include <QStandardItem>

#include "viewmanager.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
#include "RecentPlaylistsModel.h"

#include "audio/audioengine.h"
#include "dynamic/GeneratorInterface.h"
#include "playlist/playlistmodel.h"
#include "playlist/treeproxymodel.h"
#include "widgets/overlaywidget.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include <pipeline.h>

#define HISTORY_TRACK_ITEMS 25
#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_RESOLVING_TIMEOUT 2500

using namespace Tomahawk;

static QString s_whatsHotIdentifier = QString( "WhatsHotWidget" );


WhatsHotWidget::WhatsHotWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WhatsHotWidget )
{
    ui->setupUi( this );

    ui->additionsView->setFrameShape( QFrame::NoFrame );
    ui->additionsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->stackLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->breadCrumbLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );

    m_crumbModelLeft = new QStandardItemModel( this );

    ui->breadCrumbLeft->setRootIcon( QPixmap( RESPATH "images/charts.png" ) );

    connect( ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged(QModelIndex) ) );

    ui->tracksViewLeft->setFrameShape( QFrame::NoFrame );
    ui->tracksViewLeft->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->tracksViewLeft->overlay()->setEnabled( false );
    ui->tracksViewLeft->setHeaderHidden( true );
    ui->tracksViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    TreeProxyModel* artistsProxy = new TreeProxyModel( ui->artistsViewLeft );
    artistsProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
    artistsProxy->setDynamicSortFilter( true );

    ui->artistsViewLeft->setProxyModel( artistsProxy );
    ui->artistsViewLeft->setFrameShape( QFrame::NoFrame );
    ui->artistsViewLeft->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    ui->artistsViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->artistsViewLeft->header()->setVisible( false );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::PlaylistInterface* ) ), this, SLOT( playlistChanged( Tomahawk::PlaylistInterface* ) ) );
    QTimer::singleShot( 0, this, SLOT( fetchData() ) );
}


WhatsHotWidget::~WhatsHotWidget()
{
    delete ui;
}


bool
WhatsHotWidget::isBeingPlayed() const
{
    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->artistsViewLeft->playlistInterface() )
        return true;

    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->tracksViewLeft->playlistInterface() )
        return true;

    return false;
}

bool
WhatsHotWidget::jumpToCurrentTrack()
{
    if ( ui->artistsViewLeft->jumpToCurrentTrack() )
        return true;

    if ( ui->tracksViewLeft->jumpToCurrentTrack() )
        return true;

    return false;
}


void
WhatsHotWidget::fetchData()
{
    Tomahawk::InfoSystem::InfoStringHash artistInfo;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_whatsHotIdentifier;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );

    requestData.type = Tomahawk::InfoSystem::InfoChartCapabilities;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData,  20000, true );

    tDebug( LOGVERBOSE ) << "WhatsHot: requested InfoChartCapabilities";
}

void
WhatsHotWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_whatsHotIdentifier )
    {
        return;
    }

    tDebug( LOGVERBOSE ) << "WhatsHot: got something...";
    QVariantMap returnedData = output.toMap();
    qDebug() << "WhatsHot::" << returnedData;
    switch ( requestData.type )
    {
        case InfoSystem::InfoChartCapabilities:
        {
            tDebug( LOGVERBOSE ) << "WhatsHot:: info chart capabilities";
            QStandardItem *rootItem= m_crumbModelLeft->invisibleRootItem();
            tDebug( LOGVERBOSE ) << "WhatsHot:: " << returnedData.keys();

            QVariantMap defaults;
            if ( returnedData.contains( "defaults" ) )
                defaults = returnedData.take( "defaults" ).toMap();
            QString defaultSource = returnedData.take( "defaultSource" ).toString();

            foreach ( const QString label, returnedData.keys() )
            {
                tDebug( LOGVERBOSE ) << "WhatsHot:: parsing " << label;
                QStandardItem *childItem = parseNode( rootItem, label, returnedData[label] );
                tDebug( LOGVERBOSE ) << "WhatsHot:: appending" << childItem->text();
                rootItem->appendRow(childItem);
            }

            // Set the default source
            // Set the default chart for each source
            for ( int i = 0; i < rootItem->rowCount(); i++ )
            {
                QStandardItem* source = rootItem->child( i, 0 );
                if ( defaultSource.toLower() == source->text().toLower() )
                {
                    qDebug() << "Setting DEFAULT SOURCE:" << source->text();
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
                            if ( cur->child( k, 0 )->text() == index )
                            {
                                tDebug() << "Found DEFAULT ITEM:" << index;
                                cur = cur->child( k, 0 ); // this is the default, drill down into the default to pick the next default
                                cur->setData( true, Breadcrumb::DefaultRole );
                                break;
                            }
                        }
                    }
                }
            }

            ui->breadCrumbLeft->setModel( m_crumbModelLeft );
            break;
        }
        case InfoSystem::InfoChart:
        {
            if( !returnedData.contains("type") )
                break;
            const QString type = returnedData["type"].toString();
            if( !returnedData.contains(type) )
                break;
            const QString side = requestData.customData["whatshot_side"].toString();
            const QString chartId = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >().value( "chart_id" );

            m_queuedFetches.remove( chartId );
            tDebug( LOGVERBOSE ) << "WhatsHot: got chart! " << type << " on " << side;
            if( type == "artists" )
            {
                const QStringList artists = returnedData["artists"].toStringList();
                tDebug( LOGVERBOSE ) << "WhatsHot: got artists! " << artists.size();

                TreeModel* artistsModel = new TreeModel( ui->artistsViewLeft );
                artistsModel->setColumnStyle( TreeModel::TrackOnly );
                foreach ( const QString& artist, artists )
                {
                    artist_ptr artistPtr = Artist::get( artist, false );
                    artistsModel->addArtists( artistPtr );
                }

                m_artistModels[ chartId ] = artistsModel;

                setLeftViewArtists( artistsModel );
            }
            else if( type == "albums" )
            {
                QList<album_ptr> al;
                const QList<Tomahawk::InfoSystem::InfoStringHash> albums = returnedData[ "albums" ].value<QList<Tomahawk::InfoSystem::InfoStringHash> >();
                tDebug( LOGVERBOSE ) << "WhatsHot: got albums! " << albums.size();

                AlbumModel* albumModel = new AlbumModel( ui->additionsView );
                foreach ( const Tomahawk::InfoSystem::InfoStringHash& album, albums )
                {
                    qDebug() << "Getting album" << album[ "album" ] << "By" << album[ "artist" ];
                    artist_ptr artistPtr = Artist::get( album[ "artist" ], false );
                    album_ptr albumPtr = Album::get( artistPtr, album[ "album" ], false );
                    al << albumPtr;

                }
                qDebug() << "Adding albums to model";
                albumModel->addAlbums( al );

                m_albumModels[ chartId ] = albumModel;
                setLeftViewAlbums( albumModel );
            }
            else if( type == "tracks" )
            {
                const QList<Tomahawk::InfoSystem::InfoStringHash> tracks = returnedData[ "tracks" ].value<QList<Tomahawk::InfoSystem::InfoStringHash> >();
                tDebug( LOGVERBOSE ) << "WhatsHot: got tracks! " << tracks.size();

                PlaylistModel* trackModel = new PlaylistModel( ui->tracksViewLeft );
                trackModel->setStyle( TrackModel::Short );
                QList<query_ptr> tracklist;
                foreach ( const Tomahawk::InfoSystem::InfoStringHash& track, tracks )
                {
                    query_ptr query = Query::get( track[ "artist" ], track[ "track" ], QString(), uuid(), false );
                    tracklist << query;
                }
                Pipeline::instance()->resolve( tracklist );
                trackModel->append( tracklist );

                m_trackModels[ chartId ] = trackModel;
                setLeftViewTracks( trackModel );
            }
            else
            {
                tDebug( LOGVERBOSE ) << "WhatsHot: got unknown chart type" << type;
            }
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
}


void
WhatsHotWidget::leftCrumbIndexChanged( QModelIndex index )
{
    tDebug( LOGVERBOSE ) << "WhatsHot:: left crumb changed" << index.data();
    QStandardItem* item = m_crumbModelLeft->itemFromIndex( index );
    if( !item )
        return;
    if( !item->data().isValid() )
        return;


   QList<QModelIndex> indexes;
   while ( index.parent().isValid() )
   {
       indexes.prepend(index);
       index = index.parent();
   }


    const QString chartId = item->data().toString();

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
    /// Remember to lower the source!
    criteria.insert( "chart_source",  index.data().toString().toLower() );

    Tomahawk::InfoSystem::InfoRequestData requestData;
    QVariantMap customData;
    customData.insert( "whatshot_side", "left" );
    requestData.caller = s_whatsHotIdentifier;
    requestData.customData = customData;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );

    requestData.type = Tomahawk::InfoSystem::InfoChart;

    qDebug() << "Making infosystem request for chart of type:" <<chartId;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData,  20000, true );

    m_queuedFetches.insert( chartId );
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
//     tDebug( LOGVERBOSE ) << "WhatsHot:: parsing " << label;

    QStandardItem *sourceItem = new QStandardItem(label);

    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
        QList< Tomahawk::InfoSystem::InfoStringHash > charts = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
        foreach ( Tomahawk::InfoSystem::InfoStringHash chart, charts )
        {
            QStandardItem *childItem= new QStandardItem( chart[ "label" ] );
            childItem->setData( chart[ "id" ] );
            if ( chart.value( "default", "" ) == "true")
                sourceItem->setData( Breadcrumb::DefaultRole, true );
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
WhatsHotWidget::setLeftViewAlbums( AlbumModel* model )
{
    ui->additionsView->setAlbumModel( model );
    ui->additionsView->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
    ui->stackLeft->setCurrentIndex( 2 );
}


void
WhatsHotWidget::setLeftViewArtists( TreeModel* model )
{
    ui->artistsViewLeft->setTreeModel( model );
    ui->artistsViewLeft->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
    ui->stackLeft->setCurrentIndex( 1 );
}


void
WhatsHotWidget::setLeftViewTracks( PlaylistModel* model )
{
    ui->tracksViewLeft->setPlaylistModel( model );
    ui->tracksViewLeft->proxyModel()->sort( -1 );
    ui->stackLeft->setCurrentIndex( 0 );
}
