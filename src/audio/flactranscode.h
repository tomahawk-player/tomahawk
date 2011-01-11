/*! \class FLACTranscode
    \brief Transcoding plugin for FLAC streams.
*/

#ifndef FLAC_TRANSCODE_H
#define FLAC_TRANSCODE_H

#include "transcodeinterface.h"

#include <FLAC/format.h>
#include <FLAC++/decoder.h>
#include <FLAC++/metadata.h>

#include <QObject>
#include <QMutex>
#include <QDebug>

#define FLAC_BUFFER 32768 * 36
#define FLAC_BUFFER_PREFERRED 32768

class FLACTranscode : public TranscodeInterface , protected FLAC::Decoder::Stream
{
    Q_OBJECT

    public:
        FLACTranscode();
        ~FLACTranscode();

        const QStringList supportedTypes() const { QStringList l; l << "audio/flac" << "flac"; return l; }

        int needData() { return FLAC_BUFFER - m_buffer.count(); }
        bool haveData() { return !m_outBuffer.isEmpty(); }

        unsigned int preferredDataSize() { return FLAC_BUFFER_PREFERRED; }

        QByteArray data() { QByteArray b = m_outBuffer; m_outBuffer.clear(); return b; }

        QMutex* mutex() { return &m_mutex; }
        QByteArray* buffer() { return &m_buffer; }

    signals:
        void streamInitialized( long sampleRate, int channels );

    public slots:
        void onSeek( int seconds );
        void clearBuffers();
        void processData( const QByteArray& data, bool finish );

    protected:
        virtual ::FLAC__StreamDecoderReadStatus read_callback( FLAC__byte buffer[], size_t *bytes );
        virtual ::FLAC__StreamDecoderWriteStatus write_callback( const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[] );
        virtual ::FLAC__StreamDecoderSeekStatus seek_callback( FLAC__uint64 absolute_byte_offset );
        virtual bool eof_callback();
        virtual void metadata_callback( const ::FLAC__StreamMetadata *metadata );
        void error_callback( ::FLAC__StreamDecoderErrorStatus status );

    private:
        QByteArray m_outBuffer;

        QMutex m_mutex;
        QByteArray m_buffer;

        bool m_FLACRunning;
        bool m_finished;
};

#endif
