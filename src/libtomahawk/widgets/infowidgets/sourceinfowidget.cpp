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

#include "source.h"
#include "viewmanager.h"

#include "playlist/albummodel.h"
#include "playlist/collectionflatmodel.h"
#include "playlist/playlistmodel.h"

#include "database/database.h"
#include "database/databasecommand_alltracks.h"
#include "database/databasecommand_allalbums.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "widgets/overlaywidget.h"


SourceInfoWidget::SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SourceInfoWidget )
    , m_source( source )
{
    ui->setupUi( this );

    ui->historyView->setFrameShape( QFrame::NoFrame );
    ui->historyView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->recentAlbumView->setFrameShape( QFrame::NoFrame );
    ui->recentAlbumView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->recentCollectionView->setFrameShape( QFrame::NoFrame );
    ui->recentCollectionView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );

    ui->historyView->overlay()->setEnabled( false );

    m_recentCollectionModel = new CollectionFlatModel( ui->recentCollectionView );
    m_recentCollectionModel->setStyle( TrackModel::Short );
    ui->recentCollectionView->setTrackModel( m_recentCollectionModel );
    ui->recentCollectionView->sortByColumn( TrackModel::Age, Qt::DescendingOrder );
    loadTracks();

    m_historyModel = new PlaylistModel( ui->historyView );
    m_historyModel->setStyle( TrackModel::Short );
    ui->historyView->setPlaylistModel( m_historyModel );
    m_historyModel->loadHistory( source, 25 );

    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ) );

    m_recentAlbumModel = new AlbumModel( ui->recentAlbumView );
    ui->recentAlbumView->setAlbumModel( m_recentAlbumModel );
    ui->recentAlbumView->proxyModel()->sort( -1 );
    m_recentAlbumModel->addFilteredCollection( source->collection(), 20, DatabaseCommand_AllAlbums::ModificationTime );

    m_title = tr( "New Additions" );
    if ( source->isLocal() )
    {
        m_description = tr( "My recent activity" );
    }
    else
    {
        m_description = tr( "Recent activity from %1" ).arg( source->friendlyName() );
    }

    m_pixmap.load( RESPATH "images/new-additions.png" );
}


SourceInfoWidget::~SourceInfoWidget()
{
    delete ui;
}


void
SourceInfoWidget::loadTracks()
{
    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_source->collection() );
    cmd->setLimit( 250 );
    cmd->setSortOrder( DatabaseCommand_AllTracks::ModificationTime );
    cmd->setSortDescending( true );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                    SLOT( onLoadedTrackHistory( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
SourceInfoWidget::onLoadedTrackHistory( const QList<Tomahawk::query_ptr>& queries )
{
    m_recentCollectionModel->clear();
    m_recentCollectionModel->append( queries );
}


void
SourceInfoWidget::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    m_historyModel->insert( query, 0 );
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
