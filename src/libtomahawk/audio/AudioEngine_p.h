
#include <phonon/MediaObject>
#include <phonon/AudioOutput>
#include <phonon/AudioDataOutput>
#include <phonon/BackendCapabilities>
#include <phonon/Path>
#include <phonon/Effect>
#include <phonon/EffectParameter>

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
    {
    }
    AudioEngine* q_ptr;
    Q_DECLARE_PUBLIC ( AudioEngine )


public slots:
    void onStateChanged( Phonon::State newState, Phonon::State oldState );
    void onAudioDataArrived( QMap< Phonon::AudioDataOutput::Channel, QVector< qint16 > > data );

private:
    QSharedPointer<QIODevice> input;

    Tomahawk::query_ptr stopAfterTrack;
    Tomahawk::result_ptr currentTrack;
    Tomahawk::playlistinterface_ptr playlist;
    Tomahawk::playlistinterface_ptr currentTrackPlaylist;
    Tomahawk::playlistinterface_ptr queue;

    Phonon::MediaObject* mediaObject;
    Phonon::AudioOutput* audioOutput;

    Phonon::Path audioPath;
    Phonon::Effect* audioEffect;

    Phonon::AudioDataOutput* audioDataOutput;
    Phonon::Path audioDataPath;


    unsigned int timeElapsed;
    bool expectStop;
    bool waitingOnNewTrack;

    mutable QStringList supportedMimeTypes;

    AudioState state;
    QQueue< AudioState > stateQueue;
    QTimer stateQueueTimer;

    quint8 underrunCount;
    bool underrunNotified;

    QTemporaryFile* coverTempFile;

    static AudioEngine* s_instance;
};
