/*
    Copyright (C) 2012  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ANIMATED_SPINNER_H
#define ANIMATED_SPINNER_H

#include "dllmacro.h"

#include <QtGui/QWidget>

#include <QtCore/QSize>
#include <QtGui/QColor>
#include <QtGui/QPixmap>

class QTimeLine;
class QHideEvent;
class QShowEvent;
class QPaintEvent;
class QTimerEvent;

class DLLEXPORT AnimatedSpinner : public QWidget
{
    Q_OBJECT
public:
    AnimatedSpinner( QWidget *parent = 0 );

    QSize sizeHint() const;

    /**
      If you don't pass a parent QWidget, this spinner will
      draw into the pixmap instead
      */
    QPixmap pixmap() const;

public slots:
    void fadeIn();
    void fadeOut();

signals:
    void requestUpdate();

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void hideFinished();

    void frameChanged( int );

private:
    int segmentCount() const;
    QColor colorForSegment( int segment ) const;

    QTimeLine* m_showHide;
    QTimeLine* m_animation;

    int m_currentIndex;
    QPixmap m_pixmap;
};

#endif
