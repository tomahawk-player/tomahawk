/***************************************************************************
 *   Copyright (C) 2005 - 2006 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

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

// Must not be smaller than 8500 bytes!
#define OGG_BUFFER 8500
#define OGG_BUFFER_PREFERRED 32768

class VorbisTranscode : public TranscodeInterface
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
