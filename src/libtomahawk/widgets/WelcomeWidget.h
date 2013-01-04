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

#ifndef WELCOMEWIDGET_H
#define WELCOMEWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

#include "PlaylistInterface.h"

#include "Query.h"
#include "Source.h"
#include "ViewPage.h"

#include "utils/TomahawkUtilsGui.h"

#include "DllMacro.h"

class AlbumModel;
class RecentlyPlayedModel;
class OverlayWidget;
class WelcomeWidgetInterface;

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
    }

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
};


class DLLEXPORT PlaylistWidget : public QListView
{
Q_OBJECT

public:
    PlaylistWidget( QWidget* parent = 0 );

    OverlayWidget* overlay() const { return m_overlay; }

    virtual void setModel( QAbstractItemModel* model );

signals:
    void modelChanged();

private:
    OverlayWidget* m_overlay;
};


class DLLEXPORT WelcomeWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    WelcomeWidget( QWidget* parent = 0 );
    virtual ~WelcomeWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return tr( "Welcome to Tomahawk" ); }
    virtual QString description() const { return QString(); }

    virtual bool showInfoBar() const { return false; }
    virtual bool isBeingPlayed() const;

    virtual bool jumpToCurrentTrack();

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:
    void updatePlaylists();
    void updateRecentAdditions();

    void loadData();

private slots:
    void onSourcesReady();
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onPlaylistActivated( const QModelIndex& );

private:
    Ui::WelcomeWidget *ui;

    RecentlyPlayedModel* m_tracksModel;
    AlbumModel* m_recentAlbumsModel;
    Tomahawk::playlistinterface_ptr m_playlistInterface;

    friend class ::WelcomeWidgetInterface;
};

#endif // WELCOMEWIDGET_H
