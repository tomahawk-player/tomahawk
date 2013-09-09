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

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "PlaylistInterface.h"

#include "Query.h"
#include "Source.h"
#include "ViewPagePlugin.h"
#include "ViewPageLazyLoader.h"

#include "utils/TomahawkUtilsGui.h"

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

#include "../ViewPageDllMacro.h"

class AlbumModel;
class RecentlyPlayedModel;
class OverlayWidget;
class BasicHeader;

namespace Ui
{
    class DashboardWidget;
}

namespace Tomahawk
{
namespace Widgets
{


class DashboardWidget : public QWidget
{
Q_OBJECT

friend class Dashboard;

public:
    DashboardWidget( QWidget* parent = 0 );
    virtual ~DashboardWidget();

    virtual bool isBeingPlayed() const;
    virtual playlistinterface_ptr playlistInterface() const;
    virtual bool jumpToCurrentTrack();

public slots:
    void updatePlaylists();
    void updateRecentAdditions();

private slots:
    void onSourcesReady();
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onPlaylistActivated( const QModelIndex& );

protected:
    void changeEvent( QEvent* e );

private:
    Ui::DashboardWidget *ui;

    RecentlyPlayedModel* m_tracksModel;
    AlbumModel* m_recentAlbumsModel;
    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

const QString DASHBOARD_VIEWPAGE_NAME = "dashboard";

class TOMAHAWK_VIEWPAGE_EXPORT Dashboard : public Tomahawk::ViewPageLazyLoader< DashboardWidget >
{
Q_OBJECT
Q_INTERFACES( Tomahawk::ViewPagePlugin )
Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.ViewPagePlugin" )

public:
    Dashboard( QWidget* parent = 0 );
    virtual ~Dashboard();

    virtual const QString defaultName() { return DASHBOARD_VIEWPAGE_NAME; }
    virtual QString title() const { return tr( "Dashboard" ); }
    virtual QString description() const { return tr( "An overview of your friends' recent activity" ); }
    virtual const QString pixmapPath() const { return ( RESPATH "images/dashboard.svg" ); }

    virtual int sortValue() { return 1; }

    virtual bool showInfoBar() const { return true; }
};


}; // Widgets
}; // Tomahawk
#endif // DASHBOARD_H
