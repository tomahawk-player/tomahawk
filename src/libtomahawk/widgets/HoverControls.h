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

#ifndef HOVERCONTROLS_H
#define HOVERCONTROLS_H

#include <QWidget>

#include "DllMacro.h"

class QPaintEvent;

class DLLEXPORT HoverControls : public QWidget
{
    Q_OBJECT
public:
    explicit HoverControls( QWidget* parent = 0 );
    virtual ~HoverControls();

public slots:

signals:
    void play();

protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void paintEvent( QPaintEvent* event );

    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void leaveEvent( QEvent* event );

private:
    bool m_hovering;
};

#endif // HOVERCONTROLS_H
