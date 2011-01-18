#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QString>
#include <QStackedWidget>

#include "result.h"

class QAction;

class AudioControls;
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

    AudioControls* audioControls() { return m_audioControls; }
    QStackedWidget* playlistStack();

    void setWindowTitle( const QString& title );

signals:
    void settingsChanged();
    
protected:
    void changeEvent( QEvent* e );
    void closeEvent( QCloseEvent* e );

public slots:
    void createAutomaticPlaylist();
    void createStation();
    void createPlaylist();
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
    TomahawkTrayIcon* m_trayIcon;
    QNetworkAccessManager m_nam;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
};

#endif // TOMAHAWKWINDOW_H
