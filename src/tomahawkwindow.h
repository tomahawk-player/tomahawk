#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QString>

#include "tomahawk/pipeline.h"

class QAction;

class AudioControls;
class PlaylistView;
class TopBar;

typedef int AudioErrorCode; // FIXME why do we need this?

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

    PlaylistView* playlistView();
    AudioControls* audioControls() { return m_audioControls; }

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

    //void onPlaylistStatsChanged( unsigned int friends, unsigned int artists, unsigned int tracks, unsigned int filtered );

    void audioEngineError( AudioErrorCode errorCode );

private:
    void loadSettings();
    void saveSettings();
    void setupSignals();
    
    Ui::TomahawkWindow* ui;
    TopBar* m_topbar;
    AudioControls* m_audioControls;

    QNetworkAccessManager m_nam;
};

#endif // TOMAHAWKWINDOW_H
