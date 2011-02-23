/****************************************************************************************
 * Copyright (c) 2010-2011 Leo Franchi <lfranchi@kde.org>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicSetupWidget.h"

#include "ReadOrWriteWidget.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "DynamicWidget.h"
#include "source.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QPropertyAnimation>
#include <QPaintEvent>
#include <QPainter>

using namespace Tomahawk;


DynamicSetupWidget::DynamicSetupWidget( const Tomahawk::dynplaylist_ptr& playlist, QWidget* parent )
    : QWidget( parent )
    , m_playlist( playlist )
    , m_headerText( 0 )
    , m_layout( new QHBoxLayout )
    , m_generatorCombo( 0 )
    , m_logo( 0 )
    , m_generateButton( 0 )
    , m_genNumber( 0 )
{
    
    setContentsMargins( 0, 0, 0, 0 );
    m_headerText = new QLabel( tr( "Type:" ), this );
    m_layout->addWidget( m_headerText );
    
    QComboBox * genCombo = new QComboBox( this );
    foreach( const QString& type, GeneratorFactory::types() )
        genCombo->addItem( type );
    m_generatorCombo = new ReadOrWriteWidget( genCombo, m_playlist->author()->isLocal(), this );
    m_generatorCombo->setLabel( playlist->generator()->type().replace( 0, 1, playlist->generator()->type().at( 0 ).toUpper() ) );
    
    m_layout->addWidget( m_generatorCombo );
    
    m_generateButton = new QPushButton( tr( "Generate" ), this );
    m_generateButton->setAttribute( Qt::WA_LayoutUsesWidgetRect );
    connect( m_generateButton, SIGNAL( clicked( bool ) ), this, SLOT( generatePressed( bool ) ) );
    if( m_playlist->mode() == OnDemand )
        m_generateButton->hide();
    else
        m_layout->addWidget( m_generateButton );
    
    
    m_genNumber = new QSpinBox( this );
    m_genNumber->setValue( 15 );
    m_genNumber->setMinimum( 0 );
    if( m_playlist->mode() == OnDemand )
        m_genNumber->hide();
    else
        m_layout->addWidget( m_genNumber );
    
    m_layout->addSpacing( 30 );
    
    m_logo = new QLabel( this );
    if( !m_playlist->generator()->logo().isNull() ) {
        QPixmap p = m_playlist->generator()->logo().scaledToHeight( 22, Qt::SmoothTransformation );
        m_logo->setPixmap( p );
    }
    m_layout->addWidget(m_logo);
    
    setLayout( m_layout );
    
    m_fadeAnim = new QPropertyAnimation( this, "opacity" );
    m_fadeAnim->setDuration( 250 );
    m_fadeAnim->setStartValue( 0.00 );
    m_fadeAnim->setEndValue( .86 );
    
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    resize( sizeHint() );
}

DynamicSetupWidget::~DynamicSetupWidget()
{

}

void 
DynamicSetupWidget::setPlaylist( const Tomahawk::dynplaylist_ptr& playlist )
{

}

void 
DynamicSetupWidget::fadeIn()
{
    m_fadeAnim->setDirection( QAbstractAnimation::Forward );
    m_fadeAnim->start();
    
    show();
}

void 
DynamicSetupWidget::fadeOut()
{
    m_fadeAnim->setDirection( QAbstractAnimation::Backward );    
    m_fadeAnim->start();
    
}

void 
DynamicSetupWidget::generatePressed( bool )
{
    emit generatePressed( m_genNumber->value() );
}

void 
DynamicSetupWidget::setOpacity( qreal opacity )
{
    m_opacity = opacity;
    
    if( m_opacity == 0 )
        hide();
    repaint();
}

void 
DynamicSetupWidget::paintEvent( QPaintEvent* e )
{
    QPainter p( this );
    QRect r = contentsRect();
    QPalette pal = palette();
    
    DynamicWidget::paintRoundedFilledRect( p, pal, r, m_opacity );
    
    QWidget::paintEvent( e );
}
