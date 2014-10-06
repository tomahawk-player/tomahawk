/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef BACKGROUNDWIDGET_H
#define BACKGROUNDWIDGET_H

#include <QWidget>

#include "DllMacro.h"

class QPaintEvent;

class DLLEXPORT BackgroundWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackgroundWidget( QWidget* parent = 0 );
    virtual ~BackgroundWidget();

public slots:
    virtual void setBackground( const QPixmap& p, bool blurred = true, bool blackWhite = false );
    virtual void setBackgroundColor( const QColor& c );

protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void paintEvent( QPaintEvent* event );

private:
    QColor m_backgroundColor;
    QPixmap m_background;
    QPixmap m_backgroundSlice;
    bool m_blurred;
};

#endif // BACKGROUNDWIDGET_H
