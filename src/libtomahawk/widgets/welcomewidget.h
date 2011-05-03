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

#ifndef WELCOMEWIDGET_H
#define WELCOMEWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

#include "playlistinterface.h"

#include "playlist.h"
#include "result.h"
#include "viewpage.h"

#include "utils/tomahawkutils.h"

#include "dllmacro.h"

class PlaylistModel;
class OverlayWidget;

namespace Ui
{
    class WelcomeWidget;
}

class DLLEXPORT PlaylistDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistDelegate()
    {
        m_playlistIcon = QPixmap( RESPATH "images/playlist-icon.png" );
    }

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private:
    QPixmap m_playlistIcon;
};

class DLLEXPORT PlaylistWidget : public QListView
{
public:
    PlaylistWidget( QWidget* parent = 0 );

    OverlayWidget* overlay() const { return m_overlay; }

private:
    OverlayWidget* m_overlay;
};


class DLLEXPORT WelcomeWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    WelcomeWidget( QWidget* parent = 0 );
    ~WelcomeWidget();

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return 0; }

    virtual QString title() const { return tr( "Welcome to Tomahawk" ); }
    virtual QString description() const { return QString(); }

    virtual bool showStatsBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:
    void updatePlaylists();

private slots:
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onPlaylistActivated( const QModelIndex& );
    void onPlaybackFinished( const Tomahawk::query_ptr& query );

    void checkQueries();

private:
    Ui::WelcomeWidget *ui;

    PlaylistModel* m_tracksModel;
    QTimer* m_timer;
};

#endif // WELCOMEWIDGET_H
