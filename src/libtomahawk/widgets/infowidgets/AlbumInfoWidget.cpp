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

#include "AlbumInfoWidget.h"
#include "ui_AlbumInfoWidget.h"

#include "audio/audioengine.h"
#include "viewmanager.h"
#include "database/database.h"
#include "playlist/treemodel.h"
#include "playlist/albummodel.h"

#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_allalbums.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "widgets/OverlayButton.h"
#include "widgets/overlaywidget.h"

static QString s_aiInfoIdentifier = QString( "AlbumInfoWidget" );

using namespace Tomahawk;


AlbumInfoWidget::AlbumInfoWidget( const Tomahawk::album_ptr& album, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AlbumInfoWidget )
{
    ui->setupUi( this );

    ui->albumsView->setFrameShape( QFrame::NoFrame );
    ui->albumsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->tracksView->setFrameShape( QFrame::NoFrame );
    ui->tracksView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2 );

    m_albumsModel = new AlbumModel( ui->albumsView );
    ui->albumsView->setAlbumModel( m_albumsModel );

    m_tracksModel = new TreeModel( ui->tracksView );
    m_tracksModel->setMode( TreeModel::InfoSystem );
    ui->tracksView->setTreeModel( m_tracksModel );
    ui->tracksView->setRootIsDecorated( false );

    m_pixmap = QPixmap( RESPATH "images/no-album-art-placeholder.png" ).scaledToWidth( 48, Qt::SmoothTransformation );

    m_button = new OverlayButton( ui->tracksView );
    m_button->setText( tr( "Click to show Super Collection Tracks" ) );
    m_button->setCheckable( true );
    m_button->setChecked( true );

    connect( m_button, SIGNAL( clicked() ), SLOT( onModeToggle() ) );
    connect( m_tracksModel, SIGNAL( loadingStarted() ), SLOT( onLoadingStarted() ) );
    connect( m_tracksModel, SIGNAL( loadingFinished() ), SLOT( onLoadingFinished() ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    load( album );
}


AlbumInfoWidget::~AlbumInfoWidget()
{
    delete ui;
}

PlaylistInterface*
AlbumInfoWidget::playlistInterface() const
{
    return ui->tracksView->playlistInterface();
}


void
AlbumInfoWidget::onModeToggle()
{
    m_tracksModel->setMode( m_button->isChecked() ? TreeModel::InfoSystem : TreeModel::Database );
    m_tracksModel->clear();
    m_tracksModel->addTracks( m_album, QModelIndex() );

    if ( m_button->isChecked() )
        m_button->setText( tr( "Click to show Super Collection Tracks" ) );
    else
        m_button->setText( tr( "Click to show Official Tracks" ) );
}


void
AlbumInfoWidget::onLoadingStarted()
{
    m_button->setEnabled( false );
    m_button->hide();
}


void
AlbumInfoWidget::onLoadingFinished()
{
    m_button->setEnabled( true );
    m_button->show();
}

bool
AlbumInfoWidget::isBeingPlayed() const
{
    if ( ui->albumsView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->tracksView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    return false;
}

artist_ptr AlbumInfoWidget::descriptionArtist() const
{
    if ( !m_album.isNull() && !m_album->artist().isNull() )
        return m_album->artist();

    return artist_ptr();
}

ViewPage::DescriptionType
AlbumInfoWidget::descriptionType()
{
    if ( !m_album.isNull() && !m_album->artist().isNull() )
        return ViewPage::ArtistType;

    return ViewPage::TextType;
}


void
AlbumInfoWidget::load( const album_ptr& album )
{
    m_album = album;
    m_title = album->name();
    m_description = album->artist()->name();

    ui->albumsLabel->setText( tr( "Other Albums by %1" ).arg( album->artist()->name() ) );

    m_tracksModel->addTracks( album, QModelIndex() );

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums();
    cmd->setArtist( album->artist() );

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( gotAlbums( QList<Tomahawk::album_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["artist"] = album->artist()->name();
    trackInfo["album"] = album->name();

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_aiInfoIdentifier;
    requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    requestData.customData = QVariantMap();

    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
}


void
AlbumInfoWidget::gotAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    QList<Tomahawk::album_ptr> al = albums;
    if ( al.contains( m_album ) )
        al.removeAll( m_album );

    m_albumsModel->addAlbums( al );
}


void
AlbumInfoWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_aiInfoIdentifier )
    {
        return;
    }

    InfoSystem::InfoStringHash trackInfo;
    trackInfo = requestData.input.value< InfoSystem::InfoStringHash >();

    if ( output.canConvert< QVariantMap >() )
    {
        if ( trackInfo["album"] != m_album->name() )
        {
            qDebug() << "Returned info was for:" << trackInfo["album"] << "- was looking for:" << m_album->name();
            return;
        }
    }

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoAlbumCoverArt:
        {
            QVariantMap returnedData = output.value< QVariantMap >();
            const QByteArray ba = returnedData["imgbytes"].toByteArray();
            if ( ba.length() )
            {
                m_pixmap.loadFromData( ba );
                emit pixmapChanged( m_pixmap );
            }
        }

        default:
            return;
    }
}


void
AlbumInfoWidget::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
AlbumInfoWidget::changeEvent( QEvent* e )
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
