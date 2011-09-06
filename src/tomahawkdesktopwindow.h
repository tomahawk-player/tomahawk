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

#ifndef TOMAHAWKDESKTOPWINDOW_H
#define TOMAHAWKDESKTOPWINDOW_H

#include "result.h"
#include "tomahawkwindow.h"

#include <QMainWindow>
#include <QVariantMap>
#include <QPushButton>
#include <QString>
#include <QStackedWidget>

class QSearchField;
class SipPlugin;
class SourceTreeView;
class QAction;

class MusicScanner;
class AudioControls;
class TomahawkTrayIcon;
class PlaylistModel;
class QueueView;
class AnimatedSplitter;


namespace Ui
{
    class TomahawkDesktopWindow;
    class GlobalSearchWidget;
}

class TomahawkDesktopWindow : public TomahawkWindow
{
Q_OBJECT

public:
    TomahawkDesktopWindow( QWidget* parent = 0 );
    ~TomahawkDesktopWindow();

    AudioControls* audioControls() { return m_audioControls; }
    SourceTreeView* sourceTreeView() const { return m_sourcetree; }

protected:
    virtual void retranslateUi();

public slots:
    void createAutomaticPlaylist( QString );
    void createStation();
    void createPlaylist();
    void loadSpiff();
    void showSettingsDialog();
    void showDiagnosticsDialog();
    void updateCollectionManually();
    void rescanCollectionManually();
    void pluginMenuAdded(QMenu*);
    void pluginMenuRemoved(QMenu*);
    void showOfflineSources();

private slots:
    void onSipConnected();
    void onSipDisconnected();
    void onSipError();

    void addPeerManually();

    void onPlaybackLoading( const Tomahawk::result_ptr& result );

    void audioStarted();
    void audioStopped();

    void showAboutTomahawk();
    void checkForUpdates();

    void onSipPluginAdded( SipPlugin* p );
    void onSipPluginRemoved( SipPlugin* p );

    void onSearch( const QString& search );
    void onFilterEdited();

    void showQueue();
    void hideQueue();

    void playlistCreateDialogFinished( int ret );
private:
    virtual void loadSettings();
    virtual void saveSettings();

    void applyPlatformTweaks();
    void setupSignals();
    void setupSideBar();
    void setupUpdateCheck();

    Ui::TomahawkDesktopWindow* ui;
    QSearchField* m_searchWidget;
    AudioControls* m_audioControls;
    SourceTreeView* m_sourcetree;
    QPushButton* m_statusButton;
    QPushButton* m_queueButton;
    PlaylistModel* m_queueModel;
    QueueView* m_queueView;
    AnimatedSplitter* m_sidebar;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
};

#endif // TOMAHAWKDESKTOPWINDOW_H
