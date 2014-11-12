/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2014,      Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef LOADPLAYLISTDIALOG_H
#define LOADPLAYLISTDIALOG_H

#include <QDialog>

class Ui_LoadPlaylist;

class LoadPlaylistDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoadPlaylistDialog( QWidget* parent = 0, Qt::WindowFlags f = 0 );
    virtual ~LoadPlaylistDialog();

    QString url() const;
    bool autoUpdate() const;

public slots:
    void getLocalFile();

private slots:
    void onUrlChanged();

private:
    Ui_LoadPlaylist* m_ui;
};

#endif // LOADPLAYLISTDIALOG_H
