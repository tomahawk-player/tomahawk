/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "dynamic/echonest/EchonestSteerer.h"

#include "utils/tomahawkutils.h"

#include <QPaintEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <Playlist.h>
#include <QPainter>
#include <QToolButton>

using namespace Tomahawk;

#define ANIM_DURATION 300

EchonestSteerer::EchonestSteerer( QWidget* parent )
    : QWidget( parent )
    , m_layout( new QHBoxLayout )
    , m_amplifier( 0 )
    , m_field( 0 )
    , m_description( 0 )
    , m_textL( new QVBoxLayout )
    , m_steerTop( 0 )
    , m_steerBottom( 0 )
    , m_reset( 0 )
    , m_expanding( true )
    
{
    m_layout->setContentsMargins( 8, 8, 8, 8 );
    
    m_textL->setSpacing( 0 );
    m_steerTop = new QLabel( tr( "Steer this station:" ), this );
    QFont f = m_steerTop->font();
    f.setPointSize( f.pointSize() + 2 );
    f.setBold( true );
    m_steerTop->setFont( f );
    m_textL->addWidget( m_steerTop );
    m_steerBottom = new QLabel( tr( "Takes effect on track change" ), this );
    f.setPointSize( f.pointSize() - 3 );
    m_steerBottom->setFont( f );
    m_textL->addWidget( m_steerBottom );
    
    m_layout->addLayout( m_textL, 1 );
    
    m_amplifier = new QComboBox( this );
    m_amplifier->addItem( tr( "Much less" ), "^.5" );
    m_amplifier->addItem( tr( "Less" ), "^.75" );
    m_amplifier->addItem( tr( "More" ), "^1.25" );
    m_amplifier->addItem( tr( "Much more" ), "^1.5" );
    m_amplifier->addItem( tr( "Much more" ), "^1.5" );
    m_field = new QComboBox( this );
    m_field->addItem( tr( "Tempo" ), "tempo");
    m_field->addItem( tr( "Loudness" ), "loudness");
    m_field->addItem( tr( "Danceability" ), "danceability");
    m_field->addItem( tr( "Energy" ), "energy");
    m_field->addItem( tr( "Song Hotttnesss" ), "tempo");
    m_field->addItem( tr( "Artist Hotttnesss" ), "artist_hotttnesss");
    m_field->addItem( tr( "Artist Familiarity" ), "artist_familiarity");
    m_field->addItem( tr( "By Description" ), "desc");
    m_layout->addWidget( m_amplifier );
    m_layout->addWidget( m_field );
    
    connect( m_amplifier, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changed() ) );
    connect( m_field, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changed() ) );
    
    m_description = new QLineEdit( this );
    m_description->setPlaceholderText( tr( "Enter a description" ) );
    m_description->hide();
    
    m_reset = initButton( this );
    m_reset->setIcon( QIcon( RESPATH "images/view-refresh.png" ) );
    m_reset->setToolTip( tr( "Reset all steering commands" ) );
    m_layout->addWidget( m_reset );
        
    connect( m_reset, SIGNAL( clicked( bool )), this, SLOT(resetSteering(bool)));
    
    setLayout( m_layout );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    
    m_resizeAnim.setDuration( ANIM_DURATION );
    m_resizeAnim.setEasingCurve( QEasingCurve::InOutQuad );
    m_resizeAnim.setDirection( QTimeLine::Forward );
    m_resizeAnim.setUpdateInterval( 8 );
    
    connect( &m_resizeAnim, SIGNAL( frameChanged( int ) ), this, SLOT( resizeFrame( int ) ) );
    
    resize( sizeHint() );
}

void 
EchonestSteerer::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    QRect r = contentsRect();
    
    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( 0.7 );
    
    QPen pen( palette().dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( palette().highlight() );
    
    p.drawRoundedRect( r, 10, 10 );
    
    p.setOpacity( .95 );
    p.setBrush( QBrush() );
    p.setPen( pen );
    p.drawRoundedRect( r, 10, 10 );
}

void 
EchonestSteerer::changed()
{
    if( m_field->itemData( m_field->currentIndex() ).toString() != "desc" ) {
        
        QString steer = m_field->itemData( m_field->currentIndex() ).toString() + m_amplifier->itemData( m_amplifier->currentIndex() ).toString();
        emit steerField( steer );
        
        // if description was shown, animate to shrink
        if( m_layout->indexOf( m_description ) > 0 ) {
            m_expanding = false;
            int start = width();
            int end = start - m_layout->spacing() - m_description->sizeHint().width();;
            
            m_layout->removeWidget( m_description );
            m_description->hide();
            m_layout->setStretchFactor(  m_textL, 1 );
            
            m_resizeAnim.setFrameRange( start, end );
            m_resizeAnim.start();
            
            qDebug() << "COLLAPSING FROM" << start << "TO" << end;
        }
    } else { // description, so put in the description field
        if( !m_description->text().isEmpty() ) {
            QString steer = m_description->text() + m_amplifier->itemData( m_amplifier->currentIndex() ).toString();
            emit steerDescription( steer );
        }
        
        if( !m_layout->indexOf( m_description ) > 0 ) {
            // animate to expand
            m_layout->insertWidget( m_layout->count() - 1, m_description, 1 );
            m_layout->setStretchFactor( m_textL, 0 );
            m_description->show();
            
            m_expanding = true;
            int start = width();
            int end = start + m_layout->spacing() + m_description->sizeHint().width();
            m_resizeAnim.setFrameRange( start, end );
            m_resizeAnim.start();
            
            qDebug() << "EXPANDING FROM" << start << "TO" << end;
        }
    }
}

void 
EchonestSteerer::resizeFrame( int width )
{
    qDebug() << "RESIZING TO:" << width;
    resize( width, sizeHint().height() );
    update();
    
    emit resized();
}

void 
EchonestSteerer::resetSteering( bool automatic )
{
    m_field->setCurrentIndex( 0 );
    m_amplifier->setCurrentIndex( 0 );
    
    if( !automatic )
        changed();
}


QToolButton* 
EchonestSteerer::initButton( QWidget* parent )
{
    QToolButton* btn = new QToolButton( parent );
    btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    btn->setIconSize( QSize( 14, 14 ) );
    btn->setToolButtonStyle( Qt::ToolButtonIconOnly );
    btn->setAutoRaise( true );
    btn->setContentsMargins( 0, 0, 0, 0 );
    return btn;
}
