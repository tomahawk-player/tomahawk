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

#include "sourceinfowidget.h"
#include "ui_sourceinfowidget.h"

#include "utils/tomahawkutils.h"

#include "viewmanager.h"
#include "playlist/albummodel.h"
#include "playlist/collectionflatmodel.h"
#include "playlist/playlistmodel.h"

#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_allalbums.h"

#include "widgets/overlaywidget.h"


SourceInfoWidget::SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SourceInfoWidget )
{
    ui->setupUi( this );

    ui->historyView->overlay()->setEnabled( false );

    m_recentCollectionModel = new CollectionFlatModel( ui->recentCollectionView );
    ui->recentCollectionView->setTrackModel( m_recentCollectionModel );
    m_recentCollectionModel->addFilteredCollection( source->collection(), 250, DatabaseCommand_AllTracks::ModificationTime );

    m_historyModel = new PlaylistModel( ui->historyView );
    ui->historyView->setPlaylistModel( m_historyModel );
    m_historyModel->loadHistory( source );

    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ) );

    ui->recentCollectionView->setColumnHidden( TrackModel::Bitrate, true );
    ui->recentCollectionView->setColumnHidden( TrackModel::Origin, true );
    ui->recentCollectionView->setColumnHidden( TrackModel::Filesize, true );

    ui->historyView->setColumnHidden( TrackModel::Bitrate, true );
    ui->historyView->setColumnHidden( TrackModel::Origin, true );
    ui->historyView->setColumnHidden( TrackModel::Filesize, true );

    m_recentAlbumModel = new AlbumModel( ui->recentAlbumView );
    ui->recentAlbumView->setAlbumModel( m_recentAlbumModel );
    m_recentAlbumModel->addFilteredCollection( source->collection(), 20, DatabaseCommand_AllAlbums::ModificationTime );

    m_title = tr( "Info about %1" ).arg( source->isLocal() ? tr( "Your Collection" ) : source->friendlyName() );
}


SourceInfoWidget::~SourceInfoWidget()
{
    delete ui;
}


void
SourceInfoWidget::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    m_historyModel->insert( 0, query );
}


void
SourceInfoWidget::changeEvent( QEvent* e )
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
