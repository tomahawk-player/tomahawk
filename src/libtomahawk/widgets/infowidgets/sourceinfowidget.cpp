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

#include "Source.h"
#include "ViewManager.h"

#include "playlist/AlbumModel.h"
#include "playlist/CollectionFlatModel.h"
#include "playlist/RecentlyAddedModel.h"
#include "playlist/RecentlyPlayedModel.h"

#include "database/Database.h"
#include "database/databasecommand_allalbums.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "widgets/OverlayWidget.h"


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
    TomahawkUtils::unmarginLayout( ui->horizontalLayout );
    TomahawkUtils::unmarginLayout( ui->verticalLayout );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2 );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_3 );

    ui->splitter->setStretchFactor( 0, 0 );
    ui->splitter->setStretchFactor( 1, 1 );

    ui->historyView->overlay()->setEnabled( false );

    m_recentTracksModel = new RecentlyAddedModel( source, ui->recentCollectionView );
    m_recentTracksModel->setStyle( TrackModel::Short );
    ui->recentCollectionView->setTrackModel( m_recentTracksModel );
    ui->recentCollectionView->sortByColumn( TrackModel::Age, Qt::DescendingOrder );

    m_historyModel = new RecentlyPlayedModel( source, ui->historyView );
    m_historyModel->setStyle( TrackModel::Short );
    ui->historyView->setPlaylistModel( m_historyModel );

    m_recentAlbumModel = new AlbumModel( ui->recentAlbumView );
    ui->recentAlbumView->setAlbumModel( m_recentAlbumModel );
    ui->recentAlbumView->proxyModel()->sort( -1 );

    onCollectionChanged();
    connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ) );

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
SourceInfoWidget::onCollectionChanged()
{
    loadRecentAdditions();
}


void
SourceInfoWidget::loadRecentAdditions()
{
    m_recentAlbumModel->addFilteredCollection( m_source->collection(), 20, DatabaseCommand_AllAlbums::ModificationTime, true );
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
