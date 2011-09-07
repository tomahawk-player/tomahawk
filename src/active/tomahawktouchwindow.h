/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWKTOUCHWINDOW_H
#define TOMAHAWKTOUCHWINDOW_H

#include "tomahawkwindow.h"

#include <QtDeclarative>

class TomahawkTouchWindow;
class QFileSystemWatcher;

class TomahawkTouchWindow : public TomahawkWindow
{
    Q_OBJECT

public:
    TomahawkTouchWindow();
     ~TomahawkTouchWindow();


    Q_INVOKABLE void play( const QModelIndex& index );
    Q_INVOKABLE void activateItem( const QModelIndex& index );

private slots:
    void loadQml();

private:
    QDeclarativeView* m_view;
    QFileSystemWatcher* m_watcher;

};

#endif // TOMAHAWKTOUCHWINDOW_H

