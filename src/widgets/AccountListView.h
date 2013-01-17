/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef ACCOUNTLISTVIEW_H
#define ACCOUNTLISTVIEW_H

#include <QListView>

/**
 * @brief The AccountListView class does not add functionality, it just implements a
 * much needed workaround that fixes a scrolling issue with large delegates.
 */
class AccountListView : public QListView
{
    Q_OBJECT
public:
    explicit AccountListView( QWidget* parent = 0 );

protected:
    void wheelEvent( QWheelEvent* );
};

#endif // ACCOUNTLISTVIEW_H
