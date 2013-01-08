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

#ifndef WIDGET_DRAG_FILTER_H
#define WIDGET_DRAG_FILTER_H

#include "DllMacro.h"

#include <QObject>
#include <QPoint>
#include <QWidget>
#include <QPointer>

class QMouseEvent;
class QEvent;

/**
 * This class encapsulates an event filter on a widget that lets any drag events over the widget
 *  translate into move events for the whole application.
 */
class DLLEXPORT WidgetDragFilter : public QObject
{
    Q_OBJECT
public:
    explicit WidgetDragFilter(QObject* parent = 0);  

    virtual bool eventFilter(QObject* , QEvent* );
private:
    bool canDrag( QObject* obj, QMouseEvent* ev ) const;

    QPointer<QWidget> m_target; // in case it's deleted under us
    QPoint m_dragPoint;
    bool m_dragStarted;
};

#endif
