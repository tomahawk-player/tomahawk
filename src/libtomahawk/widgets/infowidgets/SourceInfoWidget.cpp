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

#include "SourceInfoWidget.h"
#include "ui_SourceInfoWidget.h"

#include "Source.h"
#include "ViewManager.h"

#include "playlist/AlbumModel.h"
#include "playlist/RecentlyAddedModel.h"
#include "playlist/RecentlyPlayedModel.h"

#include "database/Database.h"
#include "database/DatabaseCommand_AllAlbums.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"


SourceInfoWidget::SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SourceInfoWidget )
    , m_source( source )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout );
    TomahawkUtils::unmarginLayout( ui->verticalLayout );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2 );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_3 );

    ui->splitter->setStretchFactor( 0, 0 );
    ui->splitter->setStretchFactor( 1, 1 );

    m_recentTracksModel = new RecentlyAddedModel( ui->recentCollectionView );
    ui->recentCollectionView->proxyModel()->setStyle( PlayableProxyModel::Short );
    ui->recentCollectionView->setPlayableModel( m_recentTracksModel );
    ui->recentCollectionView->sortByColumn( PlayableModel::Age, Qt::DescendingOrder );
    m_recentTracksModel->setSource( source );

    m_historyModel = new RecentlyPlayedModel( ui->historyView );
    ui->historyView->proxyModel()->setStyle( PlayableProxyModel::Short );
    ui->historyView->setPlaylistModel( m_historyModel );
    m_historyModel->setSource( source );

    m_recentAlbumModel = new AlbumModel( ui->recentAlbumView );
    ui->recentAlbumView->setPlayableModel( m_recentAlbumModel );
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


QPixmap
SourceInfoWidget::pixmap() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::NewAdditions, TomahawkUtils::Original );

}
