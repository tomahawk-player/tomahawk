/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012, Teo Mrnjavac <teo@kde.org>
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

#include "ProxyStyle.h"

#include <QPainter>
#include <QSplitter>
#include <QStyleOption>
#include <QWidget>

#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"

#define ARROW_WIDTH 7
#define ARROW_HEIGHT 7


ProxyStyle::ProxyStyle( bool interceptPolish )
    : m_interceptPolish( interceptPolish )
{
}

void
ProxyStyle::polish( QPalette& pal )
{
    if( !m_interceptPolish )
        QProxyStyle::polish( pal );
}

void
ProxyStyle::drawPrimitive( PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w ) const
{
    if ( pe == PE_IndicatorBranch )
    {
        if ( opt->state & QStyle::State_Children && !w->property( "flattenBranches" ).toBool() )
        {
            int hd = ( opt->rect.height() - ARROW_HEIGHT ) / 2;
            int wd = ( opt->rect.width() - ARROW_WIDTH ) / 2;

            QRect r = opt->rect.adjusted( wd, hd, 0, 0 );
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
            p->setPen( TomahawkUtils::Colors::BORDER_LINE );
            // We must special-case this because of the AnimatedSplitterHandle which has a
            // SizeHint of 0,0.
            if( splitter->orientation() == Qt::Vertical )
                p->drawLine( opt->rect.topLeft(), opt->rect.topRight() );
            else
                p->drawLine( opt->rect.topLeft(), opt->rect.bottomRight() );
        }
    }
    else
        QProxyStyle::drawControl( ce, opt, p, w );
}

QSize
ProxyStyle::sizeFromContents( QStyle::ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget ) const
{
    if( type == CT_Splitter )
    {
        const QSplitter* splitter = qobject_cast< const QSplitter* >( widget );
        if( splitter->orientation() == Qt::Horizontal )
            return QSize( 1, size.height() );
        else
            return QSize( size.width(), 1 );
    }
    else
        return QProxyStyle::sizeFromContents( type, option, size, widget );
}
