/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef NEWPLAYLISTWIDGET_H
#define NEWPLAYLISTWIDGET_H

#include "PlaylistInterface.h"
#include "ViewPage.h"

#include "DllMacro.h"

#include <QWidget>
#include <QTimer>

class QPushButton;
class PlaylistModel;

namespace Ui
{
    class NewPlaylistWidget;
}

class DLLEXPORT NewPlaylistWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    NewPlaylistWidget( QWidget* parent = 0 );
    ~NewPlaylistWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return Tomahawk::playlistinterface_ptr(); }

    virtual QString title() const { return tr( "Create a new playlist" ); }
    virtual QString description() const { return QString(); }

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

private slots:
    void onTitleChanged( const QString& title );
    void onTagChanged();

    void updateSuggestions();
    void suggestionsFound();

    void savePlaylist();
    void cancel();

private:
    Ui::NewPlaylistWidget *ui;

    PlaylistModel* m_suggestionsModel;
    QList< Tomahawk::query_ptr > m_queries;

    QTimer m_filterTimer;
    QString m_tag;
    QPushButton* m_saveButton;
};

#endif // NEWPLAYLISTWIDGET_H
