/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

/*! \class VorbisTranscode
    \brief Transcoding plugin for OGG/Vorbis streams.
*/

#ifndef VORBIS_TRANSCODE_H
#define VORBIS_TRANSCODE_H

#include "transcodeinterface.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <QObject>
#include <QMutex>
#include <QDebug>
#include <QStringList>

#include "dllmacro.h"

// Must not be smaller than 8500 bytes!
#define OGG_BUFFER 8500
#define OGG_BUFFER_PREFERRED 32768

class DLLEXPORT VorbisTranscode : public TranscodeInterface
{
    Q_OBJECT

    public:
        VorbisTranscode();
        ~VorbisTranscode();

        const QStringList supportedTypes() const { QStringList l; l << "application/ogg" << "ogg"; return l; }

        int needData() { return OGG_BUFFER - m_buffer.count(); }
        bool haveData() { return !m_outBuffer.isEmpty(); }

        unsigned int preferredDataSize() { return OGG_BUFFER_PREFERRED; }

        QByteArray data() { QByteArray b = m_outBuffer; m_outBuffer.clear(); return b; }

        QMutex* mutex() { return &m_mutex; }
        QByteArray* buffer() { return &m_buffer; }

    public slots:
        void clearBuffers();
        void onSeek( int seconds );
        void processData( const QByteArray& data, bool finish );

    signals:
        void streamInitialized( long sampleRate, int channels );
        void timeChanged( int seconds );

    private:
        QByteArray m_outBuffer;

        QMutex m_mutex;
        QByteArray m_buffer;

        OggVorbis_File m_vorbisFile;
        bool m_vorbisInit;
};

#endif
