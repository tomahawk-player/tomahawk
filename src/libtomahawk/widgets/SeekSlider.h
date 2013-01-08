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

#ifndef SEEKSLIDER_H
#define SEEKSLIDER_H

#include "DllMacro.h"

#include <QSlider>

class QTimeLine;

class DLLEXPORT SeekSlider : public QSlider
{
Q_OBJECT

public:
    SeekSlider( QWidget* parent = 0 );
    ~SeekSlider();

    void setTimeLine( QTimeLine* timeline ) { m_timeLine = timeline; }

public slots:
    void setValue( int value );
    
protected:
    void mousePressEvent( QMouseEvent* event );

private:
    QTimeLine* m_timeLine;
};

#endif // SEEKSLIDER_H
