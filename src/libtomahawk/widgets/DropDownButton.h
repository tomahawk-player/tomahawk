/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef DROPDOWNBUTTON_H
#define DROPDOWNBUTTON_H

#include <QComboBox>

#include "Typedefs.h"
#include "DllMacro.h"

class DLLEXPORT DropDownButton : public QComboBox
{
Q_OBJECT

public:
    explicit DropDownButton( QWidget* parent = 0 );
    virtual ~DropDownButton();

    static void drawPrimitive( QPainter* p, const QRect& rect, const QString& text, bool hovering, bool itemsAvailable );

public slots:

signals:
    void clicked();

protected:
    void paintEvent( QPaintEvent* event );
    void mousePressEvent( QMouseEvent* event );

private slots:

private:
    static void setupPainter( QPainter* p );
};

#endif // DROPDOWNBUTTON_H
