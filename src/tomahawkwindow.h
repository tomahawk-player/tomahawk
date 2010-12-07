#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QString>
#include <QStackedWidget>

#include "tomahawk/pipeline.h"

class QAction;

class AudioControls;
class PlaylistManager;
class TopBar;
class TomahawkTrayIcon;

namespace Ui
{
    class TomahawkWindow;
}

class TomahawkWindow : public QMainWindow
{
Q_OBJECT

public:
    TomahawkWindow( QWidget* parent = 0 );
    ~TomahawkWindow();

    PlaylistManager* playlistManager();
    AudioControls* audioControls() { return m_audioControls; }
    QStackedWidget* playlistStack();

    void setWindowTitle( const QString& title );

signals:
    void settingsChanged();
    
protected:
    void changeEvent( QEvent* e );
    void closeEvent( QCloseEvent* e );

public slots:
    void createDynamicPlaylist();
    void createPlaylist( bool dynamic = false );
    void loadSpiff();
    void showSettingsDialog();

private slots:
    void scanFinished();
    void rescanCollectionManually();

    void onSipConnected();
    void onSipDisconnected();
    void onSipError();

    void addPeerManually();
    void addFriendManually();

    void onPlaybackLoading( const Tomahawk::result_ptr& result );

    void showAboutTomahawk();

private:
    void loadSettings();
    void saveSettings();
    void setupSignals();
    
    Ui::TomahawkWindow* ui;
    TopBar* m_topbar;
    AudioControls* m_audioControls;
    PlaylistManager* m_playlistManager;
    TomahawkTrayIcon* m_trayIcon;
    QNetworkAccessManager m_nam;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
};

#endif // TOMAHAWKWINDOW_H
