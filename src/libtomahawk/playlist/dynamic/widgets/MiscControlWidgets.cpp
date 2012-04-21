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

#include "MiscControlWidgets.h"

#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>

#include "utils/Logger.h"

using namespace Tomahawk;


LabeledSlider::LabeledSlider( const QString& leftT, const QString& rightT, QWidget* parent )
    : QWidget( parent )
{
    setLayout( new QHBoxLayout );
    layout()->setMargin( 0 );

    m_leftLabel = new QLabel( leftT, this );
    layout()->addWidget( m_leftLabel );

    m_slider = new QSlider( Qt::Horizontal, this );
    layout()->addWidget( m_slider );

    m_rightLabel = new QLabel( rightT, this );
    layout()->addWidget( m_rightLabel );
}
