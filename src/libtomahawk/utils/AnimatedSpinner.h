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

#include "DllMacro.h"

#include <QWidget>

#include <QSize>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QAbstractItemView>

class QTimeLine;
class QHideEvent;
class QShowEvent;
class QPaintEvent;
class QTimerEvent;

/**
 If you don't pass a parent QWidget, this spinner will
   draw into the pixmap instead. If you do, the pixmap will
   be invalid.

 */
class DLLEXPORT AnimatedSpinner : public QWidget
{
    Q_OBJECT

public:
    explicit AnimatedSpinner( QWidget* parent = 0 ); // widget mode
    explicit AnimatedSpinner( const QSize& size, QWidget* parent = 0 );
    AnimatedSpinner( const QSize& size, bool autoStart ); // pixmap mode

    QSize sizeHint() const;

    QPixmap pixmap() const { return m_pixmap; }
    
    void setAutoCenter( bool enabled ) { m_autoCenter = enabled; }

public slots:
    void fadeIn();
    void fadeOut();

signals:
    void requestUpdate();

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void updatePixmap();
    void hideFinished();

    void frameChanged( int );

private:
    void init();
    void drawFrame( QPainter* p, const QRect& rect );
    int segmentCount() const;
    QColor colorForSegment( int segment ) const;

    QTimeLine* m_showHide;
    QTimeLine* m_animation;

    // to tweak
    int m_radius, m_armLength, m_armWidth, m_border;
    QRect m_armRect;

    int m_currentIndex;
    QVector<qreal> m_colors;
    QPixmap m_pixmap;
    bool m_autoCenter;

    QSize m_size;
};


class LoadingSpinner : public AnimatedSpinner
{
    Q_OBJECT

public:
    explicit LoadingSpinner( QAbstractItemView* parent = 0 ); // widget mode
    
private slots:
    void onViewModelChanged();

private:
    QAbstractItemView* m_parent;
};

#endif
