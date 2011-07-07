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

#ifndef HEADERLABEL_H
#define HEADERLABEL_H

#include <QLabel>

#include "dllmacro.h"

class DLLEXPORT HeaderLabel : public QLabel
{
Q_OBJECT

public:
    HeaderLabel( QWidget* parent );
    ~HeaderLabel();

    QSize minimumSizeHint() const { return sizeHint(); }
    QSize sizeHint() const;

public slots:

protected:
//    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* event );

private:
    QWidget* m_parent;
};

#endif // HEADERLABEL_H
