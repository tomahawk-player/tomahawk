/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
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

/*! \class MadTranscode
    \brief Transcoding plugin for MP3 streams, using libmad.
*/

#ifndef MADTRANSCODE_H
#define MADTRANSCODE_H

#include "transcodeinterface.h"

#include "mad.h"

#include <QStringList>
#include <QByteArray>
#include <QObject>
#include <QMutex>

#define MP3_BUFFER 32768
#define MP3_BUFFER_PREFERRED 32768

class MADTranscode : public TranscodeInterface
{
    Q_OBJECT

    public:
        MADTranscode();
        virtual ~MADTranscode();

        const QStringList supportedTypes() const { QStringList l; l << "application/x-mp3" << "mp3"; return l; }

        int needData() { return MP3_BUFFER - m_encodedBuffer.count(); }
        bool haveData() { return !m_decodedBuffer.isEmpty(); }

        unsigned int preferredDataSize() { return MP3_BUFFER_PREFERRED; }

        QByteArray data() { QByteArray b = m_decodedBuffer; m_decodedBuffer.clear(); return b; }

        virtual void setBufferCapacity( int bytes ) { m_decodedBufferCapacity = bytes; }
        int bufferSize() { return m_decodedBuffer.size(); }

    public slots:
        virtual void clearBuffers();
        virtual void onSeek( int seconds );
        virtual void processData( const QByteArray& data, bool finish );

    signals:
        void streamInitialized( long sampleRate, int channels );
        void timeChanged( int seconds );

    private:
        QByteArray m_encodedBuffer;
        QByteArray m_decodedBuffer;
        int m_decodedBufferCapacity;

        bool m_mpegInitialised;
        struct mad_decoder decoder;
        struct mad_stream stream;
        struct mad_frame frame;
        struct mad_synth synth;
        mad_timer_t timer;
        mad_timer_t last_timer;
};

#endif
