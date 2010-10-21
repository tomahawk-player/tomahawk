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

signals:
    void settingsChanged();
    
protected:
    void changeEvent( QEvent* e );

public slots:
    void createPlaylist();
    void loadSpiff();
    void showSettingsDialog();

private slots:
    void scanFinished();
    void rescanCollectionManually();
    void addPeerManually();

private:
    void loadSettings();
    void saveSettings();
    void setupSignals();
    
    Ui::TomahawkWindow* ui;
    TopBar* m_topbar;
    AudioControls* m_audioControls;
    PlaylistManager* m_playlistManager;

    QNetworkAccessManager m_nam;
};

#endif // TOMAHAWKWINDOW_H
