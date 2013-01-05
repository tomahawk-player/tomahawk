/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "ImageButton.h"

#include "utils/Logger.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLayout>
#include <QPixmap>
#include <QIcon>
#include <QString>

ImageButton::ImageButton( QWidget* parent )
    : QAbstractButton( parent )
{
}


ImageButton::ImageButton( const QPixmap& rest, QWidget* parent )
    : QAbstractButton( parent )
{
    init( rest );
}


ImageButton::ImageButton( const QString& path, QWidget* parent )
    : QAbstractButton( parent )
{
    init( QPixmap( path ) );
}


void
ImageButton::init( const QPixmap& p )
{
    setPixmap( p, QIcon::Off );
    m_sizeHint = p.size();
    updateGeometry();
}


void
ImageButton::setPixmap( const QString& path )
{
    init( QPixmap( path ) );
}


void
ImageButton::setPixmap( const QPixmap& pixmap )
{
    init( pixmap );
}


void
ImageButton::clear()
{
    setIcon( QIcon() );
}

void
ImageButton::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    p.setClipRect( event->rect() );

    QIcon::Mode mode = isDown()
        ? QIcon::Active
        : isEnabled()
            ? QIcon::Normal
            : QIcon::Disabled;

    QIcon::State state = isChecked()
        ? QIcon::On
        : QIcon::Off;

    icon().paint( &p, rect(), Qt::AlignCenter, mode, state );
}


void
ImageButton::setPixmap( const QString& path, const QIcon::State state, const QIcon::Mode mode )
{
    setPixmap( QPixmap( path ), state, mode );
}


void
ImageButton::setPixmap( const QPixmap& p, const QIcon::State state, const QIcon::Mode mode )
{
    QIcon i = icon();
    i.addPixmap( p, mode, state );
    setIcon( i );
}
