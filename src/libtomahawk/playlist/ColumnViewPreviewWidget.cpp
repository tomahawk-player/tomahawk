/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ColumnViewPreviewWidget.h"
#include "ui_ColumnViewPreviewWidget.h"

#include <QVBoxLayout>
#include <boost/concept_check.hpp>

#include "ColumnView.h"
#include "utils/Logger.h"
#include "Source.h"
#include "utils/TomahawkUtilsGui.h"

using namespace Tomahawk;


ColumnViewPreviewWidget::ColumnViewPreviewWidget( ColumnView* parent )
    : QWidget( parent )
    , ui( new Ui::ColumnViewPreviewWidget )
{
    setVisible( false );
    ui->setupUi( this );

    ui->cover->setShowText( false );
    ui->artistLabel->setContentsMargins( 6, 2, 6, 2 );
    ui->artistLabel->setElideMode( Qt::ElideMiddle );
    ui->artistLabel->setType( QueryLabel::Artist );
    connect( ui->artistLabel, SIGNAL( clickedArtist() ), SLOT( onArtistClicked() ) );
}


ColumnViewPreviewWidget::~ColumnViewPreviewWidget()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
}


void
ColumnViewPreviewWidget::changeEvent( QEvent* e )
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


void
ColumnViewPreviewWidget::setQuery( const Tomahawk::query_ptr& query )
{
    if ( !m_query.isNull() )
    {
        disconnect( m_query->track().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
    }

    m_query = query;
    connect( m_query->track().data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );

    onCoverUpdated();
    ui->cover->setQuery( query );

    setVisible( true );

    ui->trackLabel->setText( query->track()->track() );
    ui->artistLabel->setArtist( query->track()->artistPtr() );
    ui->composerValue->setText( query->track()->composer() );

    ui->composerValue->setVisible( !query->track()->composerPtr().isNull() );
    ui->composerLabel->setVisible( !query->track()->composerPtr().isNull() );

    if ( query->numResults() )
    {
        ui->yearValue->setText( QString::number( query->track()->year() ) );
        ui->bitrateValue->setText( tr( "%1 kbps" ).arg( query->results().first()->bitrate() ) );
        ui->durationValue->setText( TomahawkUtils::timeToString( query->track()->duration() ) );
        ui->ageValue->setText( TomahawkUtils::ageToString( QDateTime::fromTime_t( query->results().first()->modificationTime() ) ) );

        ui->yearValue->setVisible( query->track()->year() > 0 );
        ui->yearLabel->setVisible( query->track()->year() > 0 );
        ui->bitrateLabel->setVisible( query->results().first()->bitrate() > 0 );
        ui->bitrateValue->setVisible( query->results().first()->bitrate() > 0 );
        ui->durationLabel->setVisible( query->track()->duration() > 0 );
        ui->durationValue->setVisible( query->track()->duration() > 0 );
        ui->ageLabel->setVisible( query->results().first()->modificationTime() > 0 );
        ui->ageValue->setVisible( query->results().first()->modificationTime() > 0 );
    }
    else
    {
        ui->yearLabel->setVisible( false );
        ui->yearValue->setVisible( false );
        ui->bitrateLabel->setVisible( false );
        ui->bitrateValue->setVisible( false );
        ui->durationLabel->setVisible( false );
        ui->durationValue->setVisible( false );
        ui->ageLabel->setVisible( false );
        ui->ageValue->setVisible( false );
    }
}


void
ColumnViewPreviewWidget::onCoverUpdated()
{
    if ( m_query->track()->cover( QSize( 0, 0 ) ).isNull() )
    {
        ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Grid, ui->cover->size() ) );
        return;
    }

    ui->cover->setPixmap( TomahawkUtils::createRoundedImage( m_query->track()->cover( ui->cover->size() ), QSize( 0, 0 ) ) );
}


QSize
ColumnViewPreviewWidget::minimumSize() const
{
    int minWidth = qMax( ui->trackLabel->sizeHint().width() + 32, ui->artistLabel->sizeHint().width() + 32 );
    return QSize( qMax( minWidth, 348 ), minimumHeight() );
}
