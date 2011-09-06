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

#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include <QMainWindow>

#include "result.h"

class TomahawkTrayIcon;


class TomahawkWindow : public QMainWindow
{
    Q_OBJECT

public:
    TomahawkWindow( QWidget* parent = 0 );
    ~TomahawkWindow();

    virtual void setWindowTitle( const QString& title );

protected:
    virtual void changeEvent( QEvent* e );
    virtual void closeEvent( QCloseEvent* e );
    virtual void showEvent( QShowEvent* e );
    virtual void hideEvent( QHideEvent* e );

    virtual void retranslateUi();
    virtual void loadSettings();
    virtual void saveSettings();

private slots:
    virtual void minimize();
    virtual void maximize();

private:
    TomahawkTrayIcon* m_trayIcon;
    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
};

#endif // TOMAHAWKWINDOW_H
