/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Teo Mrnjavac   <teo@kde.org>
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

#ifndef CONTAINEDMENUBUTTON_H
#define CONTAINEDMENUBUTTON_H

#include <QMenu>
#include <QToolButton>

/**
 * @brief The ContainedMenuButton class defines a modified QToolButton that pops
 *        up a QMenu under itself, stretching left instead of right and witout
 *        drawing an arrow primitive.
 */
class ContainedMenuButton : public QToolButton
{
    Q_OBJECT
public:
    explicit ContainedMenuButton( QWidget *parent = 0 );

    void setMenu( QMenu *menu );
    QMenu *menu() const { return m_menu; }

protected:
    void mousePressEvent( QMouseEvent *event );
private slots:
    void menuHidden();
private:
    QMenu *m_menu;
};

#endif // CONTAINEDMENUBUTTON_H
