
#include <phonon/MediaObject>
#include <phonon/AudioOutput>
#include <phonon/BackendCapabilities>

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
    {
    }
    AudioEngine* q_ptr;
    Q_DECLARE_PUBLIC ( AudioEngine )


public slots:
    void onStateChanged( Phonon::State newState, Phonon::State oldState );

private:
    QSharedPointer<QIODevice> input;

    Tomahawk::query_ptr stopAfterTrack;
    Tomahawk::result_ptr currentTrack;
    Tomahawk::playlistinterface_ptr playlist;
    Tomahawk::playlistinterface_ptr currentTrackPlaylist;
    Tomahawk::playlistinterface_ptr queue;

    Phonon::MediaObject* mediaObject;
    Phonon::AudioOutput* audioOutput;

    unsigned int timeElapsed;
    bool expectStop;
    bool waitingOnNewTrack;

    mutable QStringList supportedMimeTypes;

    AudioState state;
    QQueue< AudioState > stateQueue;
    QTimer stateQueueTimer;

    uint_fast8_t underrunCount;
    bool underrunNotified;

    QTemporaryFile* coverTempFile;

    static AudioEngine* s_instance;
};
