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

#ifndef TOMAHAWKWINDOWDECLARATIVE_H
#define TOMAHAWKWINDOWDECLARATIVE_H

#include <QtDeclarative>
#include <QMainWindow>

class TomahawkWindowDeclarative;
class QFileSystemWatcher;


class TomahawkWindowDeclarative : public QMainWindow
{
    Q_OBJECT

public:
     TomahawkWindowDeclarative();
     ~TomahawkWindowDeclarative();

public slots:
    void play( const QModelIndex& index );

// protected:
//     void changeEvent( QEvent* e );
//     void closeEvent( QCloseEvent* e );
//     void showEvent( QShowEvent* e );
//     void hideEvent( QHideEvent* e );

private slots:
    void loadQml();

private:
    QDeclarativeView* m_view;
    QFileSystemWatcher* m_watcher;

};

#endif // TOMAHAWKWINDOWDECLARATIVE_H

