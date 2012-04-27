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

#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>
#include <QPaintEvent>

#include "DllMacro.h"

/**
 * \class ComboBox
 * \brief A styled combo box that has a header background.
 */
class DLLEXPORT ComboBox : public QComboBox
{
Q_OBJECT

public:
    ComboBox( QWidget* parent = 0 );
    virtual ~ComboBox();

    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent( QPaintEvent* );
};

#endif
