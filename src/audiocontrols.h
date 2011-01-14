#ifndef AUDIOCONTROLS_H
#define AUDIOCONTROLS_H

#include <QWidget>

#include "result.h"
#include "playlistinterface.h"

namespace Ui
{
    class AudioControls;
}

class AudioControls : public QWidget
{
Q_OBJECT

public:
    AudioControls( QWidget* parent = 0 );
    ~AudioControls();

public slots:
    void onRepeatModeChanged( PlaylistInterface::RepeatMode mode );
    void onShuffleModeChanged( bool enabled );

protected:
    void changeEvent( QEvent* e );

private slots:
    void onPlaybackStarted( const Tomahawk::result_ptr& result );
    void onPlaybackLoading( const Tomahawk::result_ptr& result );
    void onPlaybackPaused();
    void onPlaybackResumed();
    void onPlaybackStopped();

    void onPlaybackTimer( qint64 msElapsed );
    void onVolumeChanged( int volume );

    void onRepeatClicked();
    void onShuffleClicked();
    void onTrackClicked();
    void onAlbumClicked();

    void onCoverArtDownloaded();

private:
    Ui::AudioControls *ui;

    QAction* m_playAction;
    QAction* m_pauseAction;
    QAction* m_prevAction;
    QAction* m_nextAction;

    QPixmap m_defaultCover;

    Tomahawk::result_ptr m_currentTrack;
    PlaylistInterface::RepeatMode m_repeatMode;
    bool m_shuffled;
};

#endif // AUDIOCONTROLS_H
