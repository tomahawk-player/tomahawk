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

#include "infobar.h"
#include "ui_infobar.h"

#include <QLabel>
#include <QPixmap>

#include "viewmanager.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include <QCheckBox>
#include <widgets/querylabel.h>

#define ANIMATION_TIME 400
#define IMAGE_HEIGHT 64

using namespace Tomahawk;


InfoBar::InfoBar( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::InfoBar )
    , m_queryLabel( 0 )
{
    ui->setupUi( this );
    TomahawkUtils::unmarginLayout( layout() );
    layout()->setContentsMargins( 8, 4, 8, 4 );

    QFont boldFont = ui->captionLabel->font();
    boldFont.setPixelSize( 18 );
    boldFont.setBold( true );
    ui->captionLabel->setFont( boldFont );
    ui->captionLabel->setElideMode( Qt::ElideRight );

    boldFont.setPixelSize( 12 );
    ui->descriptionLabel->setFont( boldFont );

    QFont regFont = ui->longDescriptionLabel->font();
    regFont.setPixelSize( 11 );
    ui->longDescriptionLabel->setFont( regFont );

    QPalette whitePal = ui->captionLabel->palette();
    whitePal.setColor( QPalette::Foreground, Qt::white );

    ui->captionLabel->setPalette( whitePal );
    ui->descriptionLabel->setPalette( whitePal );
    ui->longDescriptionLabel->setPalette( whitePal );

    ui->captionLabel->setMargin( 6 );
    ui->descriptionLabel->setMargin( 6 );
    ui->longDescriptionLabel->setMargin( 4 );

    ui->captionLabel->setText( QString() );
    ui->descriptionLabel->setText( QString() );
    ui->longDescriptionLabel->setText( QString() );
    ui->imageLabel->setText( QString() );

    m_queryLabel = new QueryLabel( this );
    m_queryLabel->setType( QueryLabel::Artist );
    m_queryLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_queryLabel->setTextPen( palette().brightText().color() );
    m_queryLabel->setFont( boldFont );
    m_queryLabel->hide();
    connect( m_queryLabel, SIGNAL( clickedArtist() ), this, SLOT( artistClicked() ) );

    m_autoUpdate = new QCheckBox( this );
    m_autoUpdate->setText( tr( "Automatically update" ) );
    m_autoUpdate->setLayoutDirection( Qt::RightToLeft );
    m_autoUpdate->setPalette( whitePal );
    connect( m_autoUpdate, SIGNAL( stateChanged( int ) ), this, SIGNAL( autoUpdateChanged( int ) ) );

    ui->horizontalLayout->addWidget( m_autoUpdate );

    m_searchWidget = new QSearchField( this );
    m_searchWidget->setPlaceholderText( tr( "Filter..." ) );
    m_searchWidget->setMinimumWidth( 180 );
    connect( m_searchWidget, SIGNAL( textChanged( QString ) ), this, SLOT( onFilterEdited() ) );

    ui->horizontalLayout->addWidget( m_searchWidget );

    QLinearGradient gradient = QLinearGradient( QPoint( 0, 0 ), QPoint( 500, 200 ) ); //HACK
    gradient.setColorAt( 0.0, QColor( 100, 100, 100 ) );
    gradient.setColorAt( 0.8, QColor( 63, 63, 63 ) );

    QPalette p = palette();
    p.setBrush( QPalette::Window, QBrush( gradient ) );
    setPalette( p );
    setAutoFillBackground( true );

    setMinimumHeight( geometry().height() );
    setMaximumHeight( geometry().height() );
    connect( ViewManager::instance(), SIGNAL( filterAvailable( bool ) ), SLOT( setFilterAvailable( bool ) ) );
    connect( ViewManager::instance(), SIGNAL( autoUpdateAvailable( bool ) ), SLOT( setAutoUpdateAvailable( bool ) ) );
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
    m_queryLabel->setQuery( Query::get( artist->name(), QString(), QString() ) );
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
InfoBar::setDescription( const album_ptr&  )
{
    // TODO
}

void
InfoBar::artistClicked()
{
    if ( m_queryLabel && !m_queryLabel->query().isNull() )
        ViewManager::instance()->show( Artist::get( m_queryLabel->artist() ) );
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
    ui->imageLabel->setPixmap( p.scaledToHeight( IMAGE_HEIGHT, Qt::SmoothTransformation ) );
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
InfoBar::setAutoUpdateAvailable( bool b )
{
    if ( b )
        m_autoUpdate->setChecked( ViewManager::instance()->currentPage()->autoUpdate() );

    m_autoUpdate->setVisible( b );
}


void
InfoBar::onFilterEdited()
{
    emit filterTextChanged( m_searchWidget->text() );
}

void
InfoBar::changeEvent( QEvent* e )
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
