/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012,      Leo Franchi            <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac           <teo@kde.org>
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

#include "InfoBar.h"
#include "ui_InfoBar.h"

#include <QLabel>
#include <QPixmap>
#include <QCheckBox>
#include <QPaintEvent>
#include <QPainter>

#include "Source.h"
#include "ViewManager.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "widgets/QueryLabel.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#define ANIMATION_TIME 400
#define IMAGE_HEIGHT 48

using namespace Tomahawk;


InfoBar::InfoBar( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::InfoBar )
    , m_queryLabel( 0 )
{
    ui->setupUi( this );

    ui->imageLabel->setMargin( 0 );
    ui->imageLabel->setFixedSize( TomahawkUtils::defaultIconSize().width() * 3,
                                  TomahawkUtils::defaultIconSize().height() * 3 );

    ui->horizontalLayout->insertSpacing( 1, TomahawkUtils::defaultIconSize().width() / 4 );
    ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 2 );

    QFont font = ui->captionLabel->font();

    int captionFontSize = TomahawkUtils::defaultFontSize() + 6;
    font.setPointSize( captionFontSize );
    font.setBold( true );
    font.setFamily( "Titillium Web" );

    ui->captionLabel->setFont( font );
    ui->captionLabel->setElideMode( Qt::ElideRight );
    ui->captionLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );
    ui->captionLabel->setMargin( 2 );
    ui->captionLabel->setMinimumHeight( QFontMetrics( font ).height() + 2 * ui->captionLabel->margin() );

    int descriptionFontSize = TomahawkUtils::defaultFontSize() + 2;
    font.setPointSize( descriptionFontSize );
    font.setBold( false );
    ui->descriptionLabel->setFont( font );
    ui->descriptionLabel->setElideMode( Qt::ElideRight );
    ui->descriptionLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );
    ui->descriptionLabel->setMargin( 2 );
    ui->descriptionLabel->setMinimumHeight( QFontMetrics( font ).height() + 2 * ui->descriptionLabel->margin() );

    QFont regFont = ui->longDescriptionLabel->font();
    regFont.setPointSize( TomahawkUtils::defaultFontSize() );
    ui->longDescriptionLabel->setFont( regFont );
    ui->longDescriptionLabel->setMargin( 4 );

    m_whitePal = ui->captionLabel->palette();
    m_whitePal.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );
    m_whitePal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND );

    ui->captionLabel->setPalette( m_whitePal );
    ui->descriptionLabel->setPalette( m_whitePal );
    ui->longDescriptionLabel->setPalette( m_whitePal );

    ui->captionLabel->setText( QString() );
    ui->descriptionLabel->setText( QString() );
    ui->longDescriptionLabel->setText( QString() );
    ui->imageLabel->setText( QString() );

    ui->lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );
    ui->lineAbove->setFrameShape( QFrame::HLine );
    ui->lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    ui->lineBelow->setFrameShape( QFrame::HLine );

    m_queryLabel = new QueryLabel( this );
    m_queryLabel->setType( QueryLabel::Artist );
    m_queryLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_queryLabel->setFont( font );
    m_queryLabel->hide();
    connect( m_queryLabel, SIGNAL( clickedArtist() ), this, SLOT( artistClicked() ) );

    m_searchWidget = new QSearchField( this );
    m_searchWidget->setPlaceholderText( tr( "Filter..." ) );
    m_searchWidget->setMinimumWidth( 220 );
    connect( m_searchWidget, SIGNAL( textChanged( QString ) ), this, SLOT( onFilterEdited() ) );

    ui->horizontalLayout->addWidget( m_searchWidget );

    QPalette pal = m_whitePal;
    pal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND );

    TomahawkUtils::unmarginLayout( ui->horizontalLayout );

    // on 72dpi, 1px = 1pt
    // margins that should be around 8 4 8 4 on ~100dpi
    int leftRightMargin = TomahawkUtils::defaultFontHeight() / 3;
    int topBottomMargin = TomahawkUtils::defaultFontHeight() / 6;

    ui->horizontalLayout->setContentsMargins( leftRightMargin, topBottomMargin,
                                              leftRightMargin, topBottomMargin );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    // top-margin + header + layout spacing + description + bottom-margin
    setFixedHeight( qMax( topBottomMargin + ui->captionLabel->height() + TomahawkUtils::defaultIconSize().height() / 4 + ui->descriptionLabel->height() + topBottomMargin,
                          topBottomMargin + ui->imageLabel->height() + topBottomMargin ) );

    setAutoFillBackground( true );
    setPalette( pal );

    connect( ViewManager::instance(), SIGNAL( filterAvailable( bool ) ), SLOT( setFilterAvailable( bool ) ) );
}


InfoBar::~InfoBar()
{
    delete ui;
}


void
InfoBar::setCaption( const QString& s )
{
    ui->captionLabel->setText( s );
}


void
InfoBar::setDescription( const QString& s )
{
    if ( m_queryLabel->isVisible() )
    {
        ui->verticalLayout->removeWidget( m_queryLabel );
        m_queryLabel->hide();

        ui->verticalLayout->addWidget( ui->descriptionLabel );
        ui->verticalLayout->setContentsMargins( 0, 0, 0, 0 );
        ui->descriptionLabel->show();
    }
    ui->descriptionLabel->setText( s );
}


void
InfoBar::setDescription( const artist_ptr& artist )
{
    m_queryLabel->setArtist( artist );
    m_queryLabel->setExtraContentsMargins( 4, 0, 0, 0 );

    if ( !m_queryLabel->isVisible() )
    {
        ui->verticalLayout->removeWidget( ui->descriptionLabel );
        ui->descriptionLabel->hide();

        m_queryLabel->show();
        ui->verticalLayout->addWidget( m_queryLabel );
        ui->verticalLayout->setContentsMargins( 0, 0, 0, 15 );
    }

}

void
InfoBar::setDescription( const album_ptr& )
{
    // TODO
}


void
InfoBar::artistClicked()
{
    if ( m_queryLabel && !m_queryLabel->artist().isNull() )
        ViewManager::instance()->show( m_queryLabel->artist() );
}


void
InfoBar::setLongDescription( const QString& s )
{
    ui->longDescriptionLabel->setText( s );

    if ( s.isEmpty() )
    {
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 1 );
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout_2, 0 );
    } else
    {
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 0 );
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout_2, 99 );
    }
}


void
InfoBar::setPixmap( const QPixmap& p )
{
    ui->imageLabel->setPixmap( p.scaledToHeight( ui->imageLabel->height(), Qt::SmoothTransformation ) );
}


void
InfoBar::setFilter( const QString& filter )
{
    m_searchWidget->setText( filter );
}


void
InfoBar::setFilterAvailable( bool b )
{
    m_searchWidget->setVisible( b );
}


void
InfoBar::setUpdaters( const QList<PlaylistUpdaterInterface*>& updaters )
{
    QList< QWidget* > newUpdaterWidgets;
    foreach ( PlaylistUpdaterInterface* updater, updaters )
    {
        if ( updater->configurationWidget() )
            newUpdaterWidgets << updater->configurationWidget();
    }

    foreach ( QWidget* updaterWidget, m_updaterConfigurations )
    {
        updaterWidget->hide();

        if ( !newUpdaterWidgets.contains( updaterWidget ) )
        {
            // Old config widget no longer present, remove it
            ui->horizontalLayout->removeWidget( updaterWidget );
        }
    }

    m_updaters = updaters;
    m_updaterConfigurations = newUpdaterWidgets;

    // Display each new widget in the proper place
    int insertIdx = -1; // Ugh, no indexOf for QSpacerItem*
    for ( int i = 0; i < ui->horizontalLayout->count(); i++ )
    {
        if ( ui->horizontalLayout->itemAt( i )->spacerItem() == ui->horizontalSpacer_4 )
        {
            insertIdx = i;
            break;
        }
    }
    insertIdx++;

    foreach ( QWidget* updaterWidget, m_updaterConfigurations )
    {
        updaterWidget->setPalette( m_whitePal );
        ui->horizontalLayout->insertWidget( insertIdx, updaterWidget );
        updaterWidget->show();
    }

//     if ( m_updaterConfiguration )
//         m_updaterConfiguration->hide();
//
//     if ( m_updaterConfiguration && ( interface ? (m_updaterConfiguration != interface->configurationWidget()) : true ) )
//         ui->horizontalLayout->removeWidget( m_updaterConfiguration );
//
//     m_updaterInterface = interface;
//     m_updaterConfiguration = interface ? interface->configurationWidget() : 0;
//
//     if ( !m_updaterInterface || !m_updaterConfiguration )
//         return;
//
//     m_updaterConfiguration->setPalette( m_whitePal );
//     int insertIdx = -1; // Ugh, no indexOf for QSpacerItem*
//     for ( int i = 0; i < ui->horizontalLayout->count(); i++ )
//     {
//         if ( ui->horizontalLayout->itemAt( i )->spacerItem() == ui->horizontalSpacer_4 )
//         {
//             insertIdx = i;
//             break;
//         }
//     }
//     insertIdx++;
//     ui->horizontalLayout->insertWidget( insertIdx, m_updaterConfiguration );
//
//     m_updaterConfiguration->show();
}


void
InfoBar::onFilterEdited()
{
    emit filterTextChanged( m_searchWidget->text() );
}


void
InfoBar::paintEvent( QPaintEvent* event )
{
    QWidget::paintEvent( event );

/*    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QLinearGradient gradient( QPoint( 0, 0 ), QPoint( 0, 1 ) );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0.0, TomahawkStyle::HEADER_LOWER );
    gradient.setColorAt( 1.0, TomahawkStyle::HEADER_UPPER );

    painter.setBrush( gradient );
    painter.fillRect( rect(), gradient );*/
}


void
InfoBar::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    switch ( event->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}
