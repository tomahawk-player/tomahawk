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

#include "ArtistInfoWidget.h"
#include "ui_ArtistInfoWidget.h"

#include "viewmanager.h"
#include "playlist/treemodel.h"
#include "playlist/playlistmodel.h"

#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_allalbums.h"

#include "utils/stylehelper.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "widgets/overlaywidget.h"

using namespace Tomahawk;


ArtistInfoWidget::ArtistInfoWidget( const Tomahawk::artist_ptr& artist, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::ArtistInfoWidget )
    , m_artist( artist )
    , m_infoId( uuid() )
{
    ui->setupUi( this );

    ui->albums->setFrameShape( QFrame::NoFrame );
    ui->albums->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->relatedArtists->setFrameShape( QFrame::NoFrame );
    ui->relatedArtists->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->topHits->setFrameShape( QFrame::NoFrame );
    ui->topHits->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget->layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget1->layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget2->layout() );
    TomahawkUtils::unmarginLayout( ui->albumHeader->layout() );

    m_albumsModel = new TreeModel( ui->albums );
    m_albumsModel->setMode( TreeModel::InfoSystem );
    ui->albums->setTreeModel( m_albumsModel );

    m_relatedModel = new TreeModel( ui->relatedArtists );
    m_relatedModel->setColumnStyle( TreeModel::TrackOnly );
    ui->relatedArtists->setTreeModel( m_relatedModel );

    m_topHitsModel = new PlaylistModel( ui->topHits );
    m_topHitsModel->setStyle( TrackModel::Short );
    ui->topHits->setTrackModel( m_topHitsModel );

    ui->albumHeader->setContentsMargins( 0, 0, 4, 0 );
    ui->button->setFixedWidth( 200 );
    ui->button->setChecked( true );

    m_pixmap = QPixmap( RESPATH "images/no-album-art-placeholder.png" ).scaledToWidth( 48, Qt::SmoothTransformation );

    connect( ui->button, SIGNAL( toggled( bool ) ), SLOT( onModeToggle( bool ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    load( artist );
}


ArtistInfoWidget::~ArtistInfoWidget()
{
    delete ui;
}


void
ArtistInfoWidget::onModeToggle( bool officialReleases )
{
    m_albumsModel->setMode( officialReleases ? TreeModel::InfoSystem : TreeModel::Database );
    m_albumsModel->clear();
    m_albumsModel->addAlbums( m_artist, QModelIndex() );
}


void
ArtistInfoWidget::load( const artist_ptr& artist )
{
    m_artist = artist;
    m_title = artist->name();
    m_albumsModel->addAlbums( artist, QModelIndex() );

    Tomahawk::InfoSystem::InfoCriteriaHash artistInfo;
    artistInfo["artist"] = artist->name();

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = m_infoId;
    requestData.customData = QVariantMap();

    requestData.input = artist->name();
    requestData.type = Tomahawk::InfoSystem::InfoArtistBiography;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( artistInfo );

    requestData.type = Tomahawk::InfoSystem::InfoArtistImages;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    requestData.type = Tomahawk::InfoSystem::InfoArtistSimilars;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    requestData.type = Tomahawk::InfoSystem::InfoArtistSongs;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
}


void
ArtistInfoWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_infoId )
        return;

    InfoSystem::InfoCriteriaHash trackInfo;
    trackInfo = requestData.input.value< InfoSystem::InfoCriteriaHash >();

    if ( output.canConvert< QVariantMap >() )
    {
        if ( trackInfo["artist"] != m_artist->name() )
        {
            qDebug() << "Returned info was for:" << trackInfo["artist"] << "- was looking for:" << m_artist->name();
            return;
        }
    }

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case InfoSystem::InfoArtistBiography:
        {
            InfoSystem::InfoGenericMap bmap = output.value< Tomahawk::InfoSystem::InfoGenericMap >();

            foreach ( const QString& source, bmap.keys() )
            {
                if ( m_longDescription.isEmpty() || source == "last.fm" )
                    m_longDescription = bmap[source]["text"];
            }
            emit longDescriptionChanged( m_longDescription );
            break;
        }

        case InfoSystem::InfoArtistSongs:
        {
            const QStringList tracks = returnedData["tracks"].toStringList();

            int i = 0;
            foreach ( const QString& track, tracks )
            {
                query_ptr query = Query::get( m_artist->name(), track, QString(), uuid() );
                m_topHitsModel->append( query );

                if ( ++i == 15 )
                    break;
            }
            break;
        }

        case InfoSystem::InfoArtistImages:
        {
            const QByteArray ba = returnedData["imgbytes"].toByteArray();
            if ( ba.length() )
            {
                QPixmap pm;
                pm.loadFromData( ba );

                if ( !pm.isNull() )
                    m_pixmap = pm.scaledToHeight( 48, Qt::SmoothTransformation );

                emit pixmapChanged( m_pixmap );
            }
            break;
        }

        case InfoSystem::InfoArtistSimilars:
        {
            const QStringList artists = returnedData["artists"].toStringList();
            foreach ( const QString& artist, artists )
            {
                m_relatedModel->addArtists( Artist::get( artist ) );
            }
            break;
        }

        default:
            return;
    }
}


void
ArtistInfoWidget::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
ArtistInfoWidget::changeEvent( QEvent* e )
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
