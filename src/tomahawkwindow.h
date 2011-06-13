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
#include <QVariantMap>
#include <QPushButton>
#include <QString>
#include <QStackedWidget>

#include "result.h"

class SipPlugin;
class SourceTreeView;
class QAction;

class MusicScanner;
class AudioControls;
class TomahawkTrayIcon;

namespace Ui
{
    class TomahawkWindow;
    class GlobalSearchWidget;
}

class TomahawkWindow : public QMainWindow
{
Q_OBJECT

public:
    TomahawkWindow( QWidget* parent = 0 );
    ~TomahawkWindow();

    AudioControls* audioControls() { return m_audioControls; }
    SourceTreeView* sourceTreeView() const { return m_sourcetree; }

    void setWindowTitle( const QString& title );

protected:
    void changeEvent( QEvent* e );
    void closeEvent( QCloseEvent* e );
    void showEvent( QShowEvent* e );
    void hideEvent( QHideEvent* e );

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

private slots:
    void onSipConnected();
    void onSipDisconnected();
    void onSipError();

    void addPeerManually();

    void onPlaybackLoading( const Tomahawk::result_ptr& result );
    void onHistoryBackAvailable( bool avail );
    void onHistoryForwardAvailable( bool avail );

    void audioStarted();
    void audioStopped();

    void showAboutTomahawk();
    void checkForUpdates();

    void onSipPluginAdded( SipPlugin* p );
    void onSipPluginRemoved( SipPlugin* p );

    void onSearch();

    void minimize();
    void maximize();

private:
    void loadSettings();
    void saveSettings();
    void setupSignals();

    Ui::TomahawkWindow* ui;
    Ui::GlobalSearchWidget* m_searchWidget;
    QWidget* m_searchBox;
    AudioControls* m_audioControls;
    TomahawkTrayIcon* m_trayIcon;
    SourceTreeView* m_sourcetree;
    QPushButton* m_statusButton;

    QAction* m_backAvailable;
    QAction* m_forwardAvailable;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
};

#endif // TOMAHAWKWINDOW_H
