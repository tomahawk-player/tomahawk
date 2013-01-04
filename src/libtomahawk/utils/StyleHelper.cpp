/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#include "StyleHelper.h"

#include <QPainter>
#include <QPixmapCache>
#include <QApplication>


QColor
StyleHelper::headerUpperColor()
{
    return QColor( "#25292c" );
}


QColor
StyleHelper::headerLowerColor()
{
    return QColor( "#707070" );
}


QColor
StyleHelper::headerHighlightColor()
{
    return QColor( "#333" );
}


void
StyleHelper::horizontalHeader( QPainter* painter, const QRect& r )
{
    painter->save();
    
    /*    QRect upperHalf( 0, 0, r.width(), r.height() / 2 );
     QRect lowerHalf( 0, upperHalf.height(), r.width(), r.height() );
     painter->fillRect( upperHalf, StyleHelper::headerUpperColor() );
     painter->fillRect( lowerHalf, StyleHelper::headerLowerColor() );*/
    QLinearGradient gradient( QPoint( 0, 0 ), QPoint( 0, 1 ) );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0.0, StyleHelper::headerLowerColor() );
    gradient.setColorAt( 1.0, StyleHelper::headerUpperColor() );
    
    painter->setBrush( gradient );
    painter->fillRect( r, gradient );
    
    /*    {
     QColor lineColor( 100, 100, 100 );
     QLine line( 0, 0, r.width(), 0 );
     painter->setPen( lineColor );
     painter->drawLine( line );
     }
     {
     QColor lineColor( 30, 30, 30 );
     QLine line( 0, r.height() - 1, r.width(), r.height() - 1 );
     painter->setPen( lineColor );
     painter->drawLine( line );
     }*/
    
    painter->restore();
}


QColor
StyleHelper::headerTextColor()
{
    return QColor( "#eaeaea" );
}


/*
 * This implementation is from QWindowsStyle (Qt 4.2)
 *
 * It is licensed under the GPL 3:
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * Contact: Nokia Corporation (qt-info@nokia.com)
 */
void StyleHelper::drawArrow( QStyle::PrimitiveElement element, QPainter* p, const QStyleOption* opt )
{
    if ( opt->rect.width() <= 1 || opt->rect.height() <= 1 )
        return;
    
    QRect r = opt->rect;
    int size = qMin( r.height(), r.width() );
    QPixmap pixmap;
    QString pixmapName;
    
    pixmapName.sprintf( "arrow-%s-%d-%d-%d-%lld", "$qt_ia", uint(opt->state), element, size, opt->palette.cacheKey() );
    if ( !QPixmapCache::find( pixmapName, pixmap ) )
    {
        int border = size / 5;
        int sqsize = 2 * ( size / 2 );
        
        QImage image( sqsize, sqsize, QImage::Format_ARGB32 );
        image.fill( 0 );
        QPainter imagePainter( &image );
        imagePainter.setRenderHint( QPainter::Antialiasing, true );
        QPolygon a;
        
        switch ( element )
        {
            case QStyle::PE_IndicatorArrowUp:
                a.setPoints( 3, border, sqsize / 2, sqsize / 2, border, sqsize - border, sqsize / 2 );
                break;
                
            case QStyle::PE_IndicatorArrowDown:
                a.setPoints( 3, border, sqsize / 2, sqsize / 2, sqsize - border,  sqsize - border, sqsize / 2 );
                break;
                
            case QStyle::PE_IndicatorArrowRight:
                a.setPoints( 3, sqsize - border, sqsize / 2, sqsize / 2, border, sqsize / 2, sqsize - border );
                break;
                
            case QStyle::PE_IndicatorArrowLeft:
                a.setPoints( 3, border, sqsize / 2, sqsize / 2, border, sqsize / 2, sqsize - border );
                break;
                
            default:
                break;
        }
        
        int bsx = 0;
        int bsy = 0;
        
        if ( opt->state & QStyle::State_Sunken )
        {
            bsx = qApp->style()->pixelMetric( QStyle::PM_ButtonShiftHorizontal );
            bsy = qApp->style()->pixelMetric( QStyle::PM_ButtonShiftVertical );
        }
        
        QRect bounds = a.boundingRect();
        int sx = sqsize / 2 - bounds.center().x() - 1;
        int sy = sqsize / 2 - bounds.center().y() - 1;
        imagePainter.translate( sx + bsx, sy + bsy );
        imagePainter.setPen( opt->palette.buttonText().color() );
        imagePainter.setBrush( opt->palette.buttonText() );
        
        if ( !( opt->state & QStyle::State_Enabled ) )
        {
            QColor foreGround( 150, 150, 150, 150 );
            imagePainter.setBrush( opt->palette.mid().color() );
            imagePainter.setPen( opt->palette.mid().color() );
        }
        else
        {
            QColor shadow( 0, 0, 0, 100 );
            imagePainter.translate( 0, 1 );
            imagePainter.setPen( shadow );
            imagePainter.setBrush( shadow );
            QColor foreGround( 255, 255, 255, 210 );
            imagePainter.drawPolygon( a );
            imagePainter.translate( 0, -1 );
            imagePainter.setPen( foreGround );
            imagePainter.setBrush( foreGround );
        }
        
        imagePainter.drawPolygon( a );
        imagePainter.end();
        
        pixmap = QPixmap::fromImage( image );
        QPixmapCache::insert( pixmapName, pixmap );
    }
    
    int xOffset = r.x() + ( r.width() - size ) / 2;
    int yOffset = r.y() + ( r.height() - size ) / 2;
    p->drawPixmap( xOffset, yOffset, pixmap );
}
