/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ToggleButton.h"

#include "utils/stylehelper.h"

#include <QStylePainter>


ToggleButton::ToggleButton( QWidget* parent )
    : QPushButton( parent )
    , m_toggled( false )
{
    setStyleSheet( QString( "QPushButton { color: white; background-color: %1; border-style: outset; border-width: 1px; border-radius: 4px; border-color: white; font: bold; } "
                            "QPushButton:pressed { background-color: %2; border-style: inset; }" )
                      .arg( StyleHelper::headerUpperColor().name() )
                      .arg( StyleHelper::headerLowerColor().darker().name() ) );

    connect( this, SIGNAL( released() ), SLOT( onToggled() ) );
}


ToggleButton::~ToggleButton()
{
}


void
ToggleButton::setDown( bool b )
{
    m_toggled = b;
    QPushButton::setDown( b );
}


void
ToggleButton::onToggled()
{
    m_toggled ^= true;
    setDown( m_toggled );

    emit toggled( m_toggled );
}


void
ToggleButton::paintEvent( QPaintEvent* event )
{
    QStylePainter p( this );
    QRect r = event->rect();
    StyleHelper::horizontalHeader( &p, r );

    QPushButton::paintEvent( event );
}
