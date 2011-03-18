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

#include "sourcetreeitemwidget.h"

#include "ui_sourcetreeitemwidget.h"

#include "tomahawk/tomahawkapp.h"
#include "album.h"
#include "database/database.h"
#include "database/databasecommand_collectionstats.h"
#include "network/dbsyncconnection.h"
#include "playlist/playlistmanager.h"

using namespace Tomahawk;


SourceTreeItemWidget::SourceTreeItemWidget( const source_ptr& source, QWidget* parent ) :
    QWidget( parent ),
    m_source( source ),
    ui( new Ui::SourceTreeItemWidget )
{
//    qDebug() << Q_FUNC_INFO;

    ui->setupUi( this );
    ui->verticalLayout->setSpacing( 3 );
    ui->activityLabel->setType( QueryLabel::ArtistAndTrack );

    QFont font = ui->nameLabel->font();
//    font.setPointSize( font.pointSize() - 1 );
    ui->nameLabel->setFont( font );

    font.setPointSize( font.pointSize() - 1 );
    ui->infoLabel->setFont( font );
    ui->activityLabel->setFont( font );

    QString displayname;
    if ( source.isNull() )
    {
        ui->avatarImage->setPixmap( QPixmap( RESPATH "images/user-avatar.png" ) );

        displayname = tr( "Super Collection" );
        ui->infoLabel->setText( tr( "All available tracks" ) );

        ui->onOffButton->hide();
    }
    else
    {
        connect( source.data(), SIGNAL( loadingStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ),
                                  SLOT( onLoadingStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ) );

        connect( source.data(), SIGNAL( stats( QVariantMap ) ), SLOT( gotStats( QVariantMap ) ) );

        connect( source.data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::query_ptr ) ) );
        connect( source.data(), SIGNAL( offline() ), SLOT( onOffline() ) );
        
        ui->avatarImage->setPixmap( QPixmap( RESPATH "images/user-avatar.png" ) );

        displayname = source->friendlyName();
        if( displayname.isEmpty() )
            displayname = source->userName();

        ui->infoLabel->setText( "???" );

        ui->onOffButton->hide();
        ui->infoButton->setPixmap( QPixmap( RESPATH "images/source-info.png" ) .scaledToHeight( 32, Qt::SmoothTransformation ) );
    }

    if ( source.isNull() || source->isLocal() )
        ui->activityLabel->setText( tr( "Idle" ) );
    else
        onOffline();

    ui->nameLabel->setText( displayname );
    ui->infoLabel->setForegroundRole( QPalette::Dark );
    ui->activityLabel->setForegroundRole( QPalette::Dark );

    ui->nameLabel->setContentsMargins( 4, 0, 0, 0 );
    ui->infoLabel->setContentsMargins( 4, 0, 0, 0 );
    ui->activityLabel->setContentsMargins( 4, 0, 0, 0 );

    connect( ui->onOffButton, SIGNAL( clicked() ), SIGNAL( clicked() ) );
    connect( ui->infoButton, SIGNAL( clicked() ), SLOT( onInfoButtonClicked() ) );
}


SourceTreeItemWidget::~SourceTreeItemWidget()
{
    qDebug() << Q_FUNC_INFO;
    delete ui;
}


void
SourceTreeItemWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
SourceTreeItemWidget::gotStats( const QVariantMap& stats )
{
    ui->infoLabel->setText( tr( "%L1 tracks" ).arg( stats.value( "numfiles" ).toInt() ) );
}


void
SourceTreeItemWidget::onLoadingStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State, const QString& info )
{
    QString msg;
    switch( newstate )
    {
        case DBSyncConnection::CHECKING:
            msg = tr( "Checking" );
            break;
        case DBSyncConnection::FETCHING:
            msg = tr( "Fetching" );
            break;
        case DBSyncConnection::PARSING:
            msg = tr( "Parsing" );
            break;
        case DBSyncConnection::SAVING:
            msg = tr( "Saving" );
            break;
        case DBSyncConnection::SYNCED:
            msg = tr( "Synced" );
            break;
        case DBSyncConnection::SCANNING:
            msg = tr( "Scanning (%L1 tracks)" ).arg( info );
            break;

        default:
            msg = "???";
    }

    ui->infoLabel->setText( msg );
}


void
SourceTreeItemWidget::onPlaybackStarted( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
    ui->activityLabel->setQuery( query );
}


void
SourceTreeItemWidget::onOnline()
{
    return;
}


void
SourceTreeItemWidget::onOffline()
{
    ui->activityLabel->setText( tr( "Offline" ) );
    return;
}


void
SourceTreeItemWidget::onInfoButtonClicked()
{
    PlaylistManager::instance()->show( m_source );
}
