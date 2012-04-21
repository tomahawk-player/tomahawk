/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christopher Reichert <creichert07@gmail.com>
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

#ifndef PLAYLISTTYPESELECTORDLG_H
#define PLAYLISTTYPESELECTORDLG_H

#include <QDialog>

#include "DllMacro.h"

namespace Ui
{
    class PlaylistTypeSelectorDlg;
}

class DLLEXPORT PlaylistTypeSelectorDlg : public QDialog
{
Q_OBJECT

public:
    PlaylistTypeSelectorDlg( QWidget* parent = 0, Qt::WindowFlags = 0 );
    ~PlaylistTypeSelectorDlg();
    bool playlistTypeIsNormal() const;
    bool playlistTypeIsAuto() const;
    QString playlistName() const;

private slots:
    void createNormalPlaylist();
    void createAutomaticPlaylist();

private:
    bool m_isAutoPlaylist; // if not an auto playlist then its a normal playlist

    Ui::PlaylistTypeSelectorDlg *ui;

};

#endif // PlaylistTypeSelectorDlg_H
