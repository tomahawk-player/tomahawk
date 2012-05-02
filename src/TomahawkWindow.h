/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QPushButton>
#include <QString>
#include <QStackedWidget>

#include "Result.h"
#include "audio/AudioEngine.h"
#include "utils/XspfLoader.h"

namespace Tomahawk {
namespace Accounts {
    class Account;
}
}

class JobStatusModel;
class QSearchField;
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
    void onAccountAdded( Tomahawk::Accounts::Account* account );
    void onAccountConnected();
    void onAccountDisconnected();
    void onAccountError();

    void onAudioEngineError( AudioEngine::AudioErrorCode error );

    void onXSPFError( XSPFLoader::XSPFErrorCode error );
    void onXSPFOk( const Tomahawk::playlist_ptr& );

    void addPeerManually();

    void onPlaybackLoading( const Tomahawk::result_ptr& result );

    void audioStarted();
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

private:
    void loadSettings();
    void saveSettings();

    void applyPlatformTweaks();
    void setupSignals();
    void setupSideBar();
    void setupUpdateCheck();

    Ui::TomahawkWindow* ui;
    QSearchField* m_searchWidget;
    AudioControls* m_audioControls;
    TomahawkTrayIcon* m_trayIcon;
    SourceTreeView* m_sourcetree;
    QPushButton* m_statusButton;
    QPushButton* m_queueButton;
    PlaylistModel* m_queueModel;
    QueueView* m_queueView;
    AnimatedSplitter* m_sidebar;
    JobStatusModel* m_jobsModel;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
    int m_audioRetryCounter;
};

#endif // TOMAHAWKWINDOW_H
