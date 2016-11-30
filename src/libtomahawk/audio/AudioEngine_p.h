#include "AudioOutput.h"

#include <stdint.h>

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QTemporaryFile>

class AudioEnginePrivate : public QObject
{
Q_OBJECT

public:
    AudioEnginePrivate( AudioEngine* q )
        : q_ptr ( q )
        , audioRetryCounter( 0 )
        , underrunCount( 0 )
        , underrunNotified( false )
    {
    }
    AudioEngine* q_ptr;
    Q_DECLARE_PUBLIC ( AudioEngine )


public slots:
    void onStateChanged( AudioOutput::AudioState newState, AudioOutput::AudioState oldState );

private:
    QSharedPointer<QIODevice> input;
    QSharedPointer<QIODevice> inputPreloaded;

    Tomahawk::query_ptr stopAfterTrack;
    Tomahawk::result_ptr currentTrack;
    Tomahawk::result_ptr preloadedTrack;
    Tomahawk::playlistinterface_ptr playlist;
    Tomahawk::playlistinterface_ptr currentTrackPlaylist;
    Tomahawk::playlistinterface_ptr queue;

    AudioOutput* audioOutput;

    unsigned int timeElapsed;
    bool waitingOnNewTrack;

    AudioState state;
    QQueue< AudioState > stateQueue;
    QTimer stateQueueTimer;

    int audioRetryCounter;
    quint8 underrunCount;
    bool underrunNotified;

    QTemporaryFile* coverTempFile;

    static AudioEngine* s_instance;
};
