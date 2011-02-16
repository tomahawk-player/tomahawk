#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QString>
#include <QStackedWidget>

#include "result.h"

class QAction;

class MusicScanner;
class AudioControls;
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

protected:
    void changeEvent( QEvent* e );
    void closeEvent( QCloseEvent* e );

public slots:
    void createAutomaticPlaylist();
    void createStation();
    void createPlaylist();
    void loadSpiff();
    void showSettingsDialog();
    void updateCollectionManually();
    
private slots:
    void onSipConnected();
    void onSipDisconnected();
    void onSipError();

    void addPeerManually();

    void onPlaybackLoading( const Tomahawk::result_ptr& result );

    void showAboutTomahawk();

private:
    void loadSettings();
    void saveSettings();
    void setupSignals();
    
    Ui::TomahawkWindow* ui;
    AudioControls* m_audioControls;
    TomahawkTrayIcon* m_trayIcon;
    QNetworkAccessManager m_nam;
    QPushButton* m_statusButton;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
};

#endif // TOMAHAWKWINDOW_H
