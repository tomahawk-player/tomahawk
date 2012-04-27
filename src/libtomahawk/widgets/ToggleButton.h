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

#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include <QLabel>
#include <QPaintEvent>

#include "DllMacro.h"

/**
 * \class ToggleButton
 * \brief A styled toggle-button that has a header background.
 */
class DLLEXPORT ToggleButton : public QLabel
{
Q_OBJECT

public:
    ToggleButton( QWidget* parent = 0 );
    virtual ~ToggleButton();

    QSize minimumSizeHint() const { return sizeHint(); }

    bool isChecked() const { return m_checked; }

public slots:
    virtual void setText( const QString& text );
    virtual void setChecked( bool b ) { m_checked = b; }

signals:
    void clicked();

protected:
    virtual void paintEvent( QPaintEvent* );
    void mouseReleaseEvent( QMouseEvent* event );

private:
    bool m_checked;
};

#endif
