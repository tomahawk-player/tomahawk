/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013-2014, Teo Mrnjavac <teo@kde.org>
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

#include "ColumnView.h"
#include "widgets/PlayableCover.h"
#include "widgets/QueryLabel.h"
#include "widgets/ScrollingLabel.h"
#include "utils/Logger.h"
#include "Source.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/DpiScaler.h"
#include "ViewManager.h"

#include <QLabel>
#include <QVBoxLayout>
#include <boost/concept_check.hpp>

using namespace Tomahawk;


ColumnViewPreviewWidget::ColumnViewPreviewWidget( ColumnView* parent )
    : QWidget( parent )
{
    setVisible( false );

    QBoxLayout* mainLayout = new QVBoxLayout;
    setLayout( mainLayout );

#ifndef Q_OS_MAC //we don't need to scale on OSX anyway
    mainLayout->addSpacing( TomahawkUtils::DpiScaler::scaledY( this, 8 ) );
#else
    mainLayout->addSpacing( 8 );
#endif

    QBoxLayout* coverCenterLayout = new QHBoxLayout;
    mainLayout->addLayout( coverCenterLayout );

    m_cover = new PlayableCover( this );
    m_cover->setShowText( false );
    m_cover->setFixedSize( 260, 260 );
    m_cover->setAlignment( Qt::AlignCenter );

    coverCenterLayout->addStretch();
    coverCenterLayout->addWidget( m_cover );
    coverCenterLayout->addStretch();

#ifndef Q_OS_MAC //we don't need to scale on OSX anyway
    mainLayout->addSpacing( TomahawkUtils::DpiScaler::scaledY( this, 16 ) );
#else
    mainLayout->addSpacing( 16 );
#endif

    m_trackLabel = new ScrollingLabel( this );
    QFont font;
    font.setPointSize( TomahawkUtils::defaultFontSize() + 9 );
    font.setBold( true );
    m_trackLabel->setFont( font );
    m_trackLabel->setFixedHeight( QFontMetrics( font ).height() + 6 );
    m_trackLabel->setAlignment( Qt::AlignCenter );
    QHBoxLayout* trackLayout = new QHBoxLayout;
    trackLayout->addSpacing( 3 );
    trackLayout->addWidget( m_trackLabel );
    trackLayout->addSpacing( 3 );
    mainLayout->addLayout( trackLayout );

    m_artistLabel = new QueryLabel( this );
    m_artistLabel->setContentsMargins( 6, 2, 6, 2 );
    m_artistLabel->setElideMode( Qt::ElideMiddle );
    m_artistLabel->setType( QueryLabel::Artist );
    m_artistLabel->setAlignment( Qt::AlignCenter );
    connect( m_artistLabel, SIGNAL( clickedArtist() ), SLOT( onArtistClicked() ) );
    font.setPointSize( TomahawkUtils::defaultFontSize() + 5 );
    m_artistLabel->setFont( font );
    QHBoxLayout* artistLayout = new QHBoxLayout;
    artistLayout->addStretch();
    artistLayout->addWidget( m_artistLabel );
    m_artistLabel->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    artistLayout->addStretch();
    mainLayout->addLayout( artistLayout );

#ifndef Q_OS_MAC //we don't need to scale on OSX anyway
    mainLayout->addSpacing( TomahawkUtils::DpiScaler::scaledY( this, 16 ) );
#else
    mainLayout->addSpacing( 16 );
#endif

    QGridLayout* gridLayout = new QGridLayout;
    mainLayout->addLayout( gridLayout );

    font.setPointSize( TomahawkUtils::defaultFontSize() + 3 );

    m_composerLabel = new QLabel( this );
    m_composerLabel->setEnabled( false );
    m_composerLabel->setFont( font );
    m_composerLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_composerLabel->setText( tr( "Composer:" ) );
    gridLayout->addWidget( m_composerLabel, 0, 0 );

    m_durationLabel = new QLabel( this );
    m_durationLabel->setEnabled( false );
    m_durationLabel->setFont( font );
    m_durationLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_durationLabel->setText( tr( "Duration:" ) );
    gridLayout->addWidget( m_durationLabel, 1, 0 );

    m_bitrateLabel = new QLabel( this );
    m_bitrateLabel->setEnabled( false );
    m_bitrateLabel->setFont( font );
    m_bitrateLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_bitrateLabel->setText( tr( "Bitrate:" ) );
    gridLayout->addWidget( m_bitrateLabel, 2, 0 );

    m_yearLabel = new QLabel( this );
    m_yearLabel->setEnabled( false );
    m_yearLabel->setFont( font );
    m_yearLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_yearLabel->setText( tr( "Year:" ) );
    gridLayout->addWidget( m_yearLabel, 3, 0 );

    m_ageLabel = new QLabel( this );
    m_ageLabel->setEnabled( false );
    m_ageLabel->setFont( font );
    m_ageLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_ageLabel->setText( tr( "Age:" ) );
    gridLayout->addWidget( m_ageLabel, 4, 0 );

    m_composerValue = new QLabel( this );
    m_composerValue->setFont( font );
    m_composerValue->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    gridLayout->addWidget( m_composerValue, 0, 1 );

    m_durationValue = new QLabel( this );
    m_durationValue->setFont( font );
    m_durationValue->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    gridLayout->addWidget( m_durationValue, 1, 1 );

    m_bitrateValue = new QLabel( this );
    m_bitrateValue->setFont( font );
    m_bitrateValue->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    gridLayout->addWidget( m_bitrateValue, 2, 1 );

    m_yearValue = new QLabel( this );
    m_yearValue->setFont( font );
    m_yearValue->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    gridLayout->addWidget( m_yearValue, 3, 1 );

    m_ageValue = new QLabel( this );
    m_ageValue->setFont( font );
    m_ageValue->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    gridLayout->addWidget( m_ageValue, 4, 1 );

    mainLayout->addStretch();

    TomahawkUtils::unmarginLayout( mainLayout );

#ifndef Q_OS_MAC //we don't need to scale on OSX anyway
    gridLayout->setSpacing( TomahawkUtils::DpiScaler::scaledX( this, 4 ) );
#else
    gridLayout->setSpacing( 4 );
#endif
}


ColumnViewPreviewWidget::~ColumnViewPreviewWidget()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
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
    m_cover->setQuery( query );

    setVisible( true );

    m_trackLabel->setText( query->track()->track() );
    m_artistLabel->setArtist( query->track()->artistPtr() );
    m_artistLabel->setMinimumWidth( qMin( m_artistLabel->fontMetrics().width( query->track()->artist() ) +
                                          m_artistLabel->contentsMargins().left() +
                                          m_artistLabel->contentsMargins().right() +
                                          2 * m_artistLabel->lineWidth(),
                                          width() ) );
    m_artistLabel->setElideMode( Qt::ElideRight );
    m_composerValue->setText( query->track()->composer() );

    m_composerValue->setVisible( !query->track()->composerPtr().isNull() );
    m_composerLabel->setVisible( !query->track()->composerPtr().isNull() );

    if ( query->numResults() )
    {
        m_yearValue->setText( QString::number( query->track()->year() ) );
        m_bitrateValue->setText( tr( "%1 kbps" ).arg( query->results().first()->bitrate() ) );
        m_durationValue->setText( TomahawkUtils::timeToString( query->track()->duration() ) );
        m_ageValue->setText( TomahawkUtils::ageToString( QDateTime::fromTime_t( query->results().first()->modificationTime() ) ) );

        m_yearValue->setVisible( query->track()->year() > 0 );
        m_yearLabel->setVisible( query->track()->year() > 0 );
        m_bitrateLabel->setVisible( query->results().first()->bitrate() > 0 );
        m_bitrateValue->setVisible( query->results().first()->bitrate() > 0 );
        m_durationLabel->setVisible( query->track()->duration() > 0 );
        m_durationValue->setVisible( query->track()->duration() > 0 );
        m_ageLabel->setVisible( query->results().first()->modificationTime() > 0 );
        m_ageValue->setVisible( query->results().first()->modificationTime() > 0 );
    }
    else
    {
        m_yearLabel->setVisible( false );
        m_yearValue->setVisible( false );
        m_bitrateLabel->setVisible( false );
        m_bitrateValue->setVisible( false );
        m_durationLabel->setVisible( false );
        m_durationValue->setVisible( false );
        m_ageLabel->setVisible( false );
        m_ageValue->setVisible( false );
    }

#ifndef Q_OS_MAC //we don't need to scale on OSX anyway
    TomahawkUtils::DpiScaler dpi( this );
    setMinimumHeight( dpi.scaledY( 400 ) );
#else
    setMinimumHeight( 400 );
#endif
}


void
ColumnViewPreviewWidget::onCoverUpdated()
{
    if ( m_query->track()->cover( QSize( 0, 0 ) ).isNull() )
    {
        m_cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Grid, m_cover->size() ) );
        return;
    }

    m_cover->setPixmap( TomahawkUtils::createRoundedImage( m_query->track()->cover( m_cover->size() ), QSize( 0, 0 ) ) );
}


void
ColumnViewPreviewWidget::onArtistClicked()
{
    if ( !m_query->track()->artistPtr().isNull() )
        ViewManager::instance()->show( m_query->track()->artistPtr() );
}


QSize
ColumnViewPreviewWidget::minimumSize() const
{
    int minWidth = TomahawkUtils::DpiScaler::scaledX( this, 280 );
    return QSize( qMax( minWidth, 348 ), minimumHeight() );
}
