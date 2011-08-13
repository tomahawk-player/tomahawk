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

#include "proxystyle.h"

#include <QPainter>
#include <QSplitter>
#include <QStyleOption>
#include <QWidget>

#include "utils/logger.h"

#define ARROW_WIDTH 7
#define ARROW_HEIGHT 7


void
ProxyStyle::drawPrimitive( PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w ) const
{
    if ( pe == PE_IndicatorBranch )
    {
        if ( opt->state & QStyle::State_Children )
        {
            QRect r = opt->rect;

            int hd = ( opt->rect.height() - ARROW_HEIGHT ) / 2;
            int wd = ( opt->rect.width() - ARROW_WIDTH ) / 2;
            r.adjust( wd, hd, 0, 0 );

            QPointF pointsOpened[3] = { QPointF( r.x(), r.y() ), QPointF( r.x() + ARROW_WIDTH, r.y() ), QPointF( r.x() + ARROW_WIDTH / 2, r.y() + ARROW_HEIGHT ) };
            QPointF pointsClosed[3] = { QPointF( r.x(), r.y() ), QPointF( r.x() + ARROW_WIDTH, r.y() + ARROW_HEIGHT / 2 ), QPointF( r.x(), r.y() + ARROW_HEIGHT ) };

            p->save();
            p->setRenderHint( QPainter::Antialiasing );

            p->setPen( opt->palette.dark().color() );
            p->setBrush( opt->palette.dark().color() );
            if ( !( opt->state & QStyle::State_Open ) )
            {
                p->drawPolygon( pointsClosed, 3 );
            }
            else
            {
                p->drawPolygon( pointsOpened, 3 );
            }

            p->restore();
        }
        return;
    }

    if ( pe != PE_FrameStatusBar )
        QProxyStyle::drawPrimitive( pe, opt, p, w );
}


void
ProxyStyle::drawControl( ControlElement ce, const QStyleOption* opt, QPainter* p, const QWidget* w ) const
{
    if ( ce == CE_Splitter )
    {
        const QSplitter* splitter = qobject_cast< const QSplitter* >( w );
        if ( !splitter->sizes().contains( 0 ) )
        {
            p->setPen( QColor( 0x8c, 0x8c, 0x8c ) );
            p->drawLine( opt->rect.topLeft(), opt->rect.bottomRight() );
        }
        else
        {
            p->setPen( QColor( 0xff, 0xff, 0xff ) );
            p->drawLine( opt->rect.topLeft(), opt->rect.bottomRight() );
        }
    }
    else
        QProxyStyle::drawControl( ce, opt, p, w );
}
