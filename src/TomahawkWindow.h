/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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
#include <QToolButton>

#include "config.h"
#include "Result.h"
#include "audio/AudioEngine.h"
#include "utils/XspfLoader.h"

#ifdef Q_OS_WIN
    #include <shobjidl.h>
#endif

class SettingsDialog;
namespace Tomahawk
{
    namespace Accounts
    {
        class Account;
    }
}

class JobStatusSortModel;
class QSearchField;
class SourceTreeView;
class QAction;

class MusicScanner;
class AudioControls;
class TomahawkTrayIcon;
class PlaylistModel;
class QueueView;
class AnimatedSplitter;

class AccountsToolButton;

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
    void keyPressEvent( QKeyEvent* e );

#ifdef Q_OS_WIN
    bool winEvent( MSG* message, long* result );
#endif

public slots:
    void createAutomaticPlaylist( QString );
    void createStation();
    void createPlaylist();
    void loadSpiff();
    void showSettingsDialog();
    void showDiagnosticsDialog();
    void legalInfo();
    void updateCollectionManually();
    void rescanCollectionManually();
    void showOfflineSources();

    void fullScreenEntered();
    void fullScreenExited();

private slots:
    void onAccountError();

    void onHistoryBackAvailable( bool avail );
    void onHistoryForwardAvailable( bool avail );

    void onAudioEngineError( AudioEngine::AudioErrorCode error );

    void onXSPFError( XSPFLoader::XSPFErrorCode error );
    void onXSPFOk( const Tomahawk::playlist_ptr& );

    void addPeerManually();

    void onPlaybackLoading( const Tomahawk::result_ptr& result );

    void audioStarted();
    void audioFinished();
    void audioPaused();
    void audioStopped();

    void showAboutTomahawk();
    void checkForUpdates();

    void onSearch( const QString& search );
    void onFilterEdited();

    void loadXspfFinished( int );

    void showQueue();
    void hideQueue();

    void minimize();
    void maximize();

    void playlistCreateDialogFinished( int ret );

    void crashNow();

    void toggleMenuBar();
    void balanceToolbar();

#ifdef Q_OS_WIN
    void audioStateChanged( AudioState newState, AudioState oldState );
    void updateWindowsLoveButton();
#endif

private:
    void loadSettings();
    void saveSettings();

    void applyPlatformTweaks();
    void setupSignals();
    void setupMenuBar();
    void setupToolBar();
    void setupSideBar();
    void setupUpdateCheck();

#ifdef Q_OS_WIN
    bool setupWindowsButtons();
    const unsigned int m_buttonCreatedID;
  #ifdef HAVE_THUMBBUTTON
    ITaskbarList3* m_taskbarList;
    THUMBBUTTON m_thumbButtons[5];
  #endif
    enum TB_STATES{ TP_PREVIOUS = 0,TP_PLAY_PAUSE = 1,TP_NEXT = 2,TP_LOVE = 4 };
#endif

    Ui::TomahawkWindow* ui;
    QSearchField* m_searchWidget;
    AudioControls* m_audioControls;
    TomahawkTrayIcon* m_trayIcon;
    SourceTreeView* m_sourcetree;
    QPushButton* m_statusButton;
    QPushButton* m_queueButton;
    QueueView* m_queueView;
    AnimatedSplitter* m_sidebar;
    JobStatusSortModel* m_jobsModel;
    SettingsDialog* m_settingsDialog;

    // Menus and menu actions: Accounts menu
    QMenuBar    *m_menuBar;
#ifndef Q_OS_MAC
    QAction     *m_compactMenuAction;
    QMenu       *m_compactMainMenu;
#endif
    AccountsToolButton *m_accountsButton;
    QToolBar *m_toolbar;
    QWidget *m_toolbarLeftBalancer;
    QWidget *m_toolbarRightBalancer;

    QAction* m_backAction;
    QAction* m_forwardAction;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
    int m_audioRetryCounter;
};

#endif // TOMAHAWKWINDOW_H
