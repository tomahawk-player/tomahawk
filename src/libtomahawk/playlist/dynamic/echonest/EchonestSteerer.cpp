/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "playlist/dynamic/echonest/EchonestSteerer.h"

#include "Source.h"
#include "playlist/dynamic/widgets/DynamicWidget.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#include <echonest/Playlist.h>

#include <QPaintEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QPropertyAnimation>

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
//    m_steerBottom = new QLabel( tr( "Takes effect on track change" ), this );
//    f.setPointSize( f.pointSize() - 3 );
//    m_steerBottom->setFont( f );
//    m_textL->addWidget( m_steerBottom );


    QPalette p = m_steerTop->palette();
#ifdef Q_OS_MAC
    p.setBrush( QPalette::WindowText, Qt::white );
#else
    p.setBrush( QPalette::WindowText, palette().highlightedText()  );
#endif
    m_steerTop->setPalette( p );

    m_layout->addLayout( m_textL, 1 );

    m_amplifier = new QComboBox( this );
    m_amplifier->addItem( tr( "Much less" ), "^.1" );
    m_amplifier->addItem( tr( "Less" ), "^.5" );
    m_amplifier->addItem( tr( "A bit less" ), "^.75" );
    m_amplifier->addItem( tr( "Keep at current", "" ) );
    m_amplifier->addItem( tr( "A bit more" ), "^1.25" );
    m_amplifier->addItem( tr( "More" ), "^1.5" );
    m_amplifier->addItem( tr( "Much more" ), "^2" );
    m_amplifier->setCurrentIndex( 3 );
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

    connect( m_description, SIGNAL( textChanged( QString ) ), this, SLOT( changed() ) );

    m_apply = initButton( this );
    m_apply->setIcon( ImageRegistry::instance()->icon( RESPATH "images/apply-check.svg" ) );
    m_apply->setToolTip( tr( "Apply steering command" ) );
    m_layout->addWidget( m_apply );
    connect( m_apply, SIGNAL( clicked( bool ) ), this, SLOT( applySteering() ) );

    m_reset = initButton( this );
    m_reset->setIcon( ImageRegistry::instance()->icon( RESPATH "images/view-refresh.svg" ) );
    m_reset->setToolTip( tr( "Reset all steering commands" ) );
    m_layout->addWidget( m_reset );

    connect( m_reset, SIGNAL( clicked( bool ) ), this, SLOT( resetSteering( bool ) ) );

    setLayout( m_layout );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    m_resizeAnim.setDuration( ANIM_DURATION );
    m_resizeAnim.setEasingCurve( QEasingCurve::InOutQuad );
    m_resizeAnim.setDirection( QTimeLine::Forward );
    m_resizeAnim.setUpdateInterval( 8 );

    connect( &m_resizeAnim, SIGNAL( frameChanged( int ) ), this, SLOT( resizeFrame( int ) ) );


    m_fadeAnim = new QPropertyAnimation( this, "opacity", this );
    m_fadeAnim->setDuration( ANIM_DURATION );
    m_fadeAnim->setStartValue( 0 );
    m_fadeAnim->setEndValue( .7 );
    resize( sizeHint() );
}


void
EchonestSteerer::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    QRect r = contentsRect();
    QPalette pal = palette();

    DynamicWidget::paintRoundedFilledRect( p, pal, r, m_opacity );
}


void
EchonestSteerer::setOpacity( qreal opacity )
{
    m_opacity = opacity;
    if( m_opacity == 0 )
        hide();
    repaint();
}


void
EchonestSteerer::fadeIn()
{
    m_fadeAnim->setDirection( QAbstractAnimation::Forward );
    m_fadeAnim->start();

    show();
}


void
EchonestSteerer::fadeOut()
{
    m_fadeAnim->setDirection( QAbstractAnimation::Backward );
    m_fadeAnim->start();
}


void
EchonestSteerer::changed()
{
    if( m_field->itemData( m_field->currentIndex() ).toString() != "desc" ) {
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
        }
    } else { // description, so put in the description field
        if( m_layout->indexOf( m_description ) == -1 ) {
            // animate to expand
            m_layout->insertWidget( m_layout->count() - 1, m_description, 1 );
            m_layout->setStretchFactor( m_textL, 0 );
            m_description->show();

            m_expanding = true;
            int start = width();
            int end = start + m_layout->spacing() + m_description->sizeHint().width();
            m_resizeAnim.setFrameRange( start, end );
            m_resizeAnim.start();
        }
    }
}

void
EchonestSteerer::applySteering()
{
    if ( m_field->itemData( m_field->currentIndex() ).toString() != "desc" )
    {
        QString steer = m_field->itemData( m_field->currentIndex() ).toString() + m_amplifier->itemData( m_amplifier->currentIndex() ).toString();
        emit steerField( steer );
    }
    else
    {
        if ( !m_description->text().isEmpty() )
        {
            QString steer = m_description->text() + m_amplifier->itemData( m_amplifier->currentIndex() ).toString();
            emit steerDescription( steer );
        }
    }

    emit steeringChanged();

    resetSteering( true );
}


void
EchonestSteerer::resizeFrame( int width )
{
//     qDebug() << "RESIZING TO:" << width;
    resize( width, sizeHint().height() );
    repaint();

    emit resized();
}


void
EchonestSteerer::resetSteering( bool automatic )
{
    m_amplifier->setCurrentIndex( 3 );

    if( !automatic ) {
        m_description->clear();
        m_field->setCurrentIndex( 0 );
        emit reset();
    }
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
