/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013       Dominik Schmidt <domme@tomahawk-player.org>
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
