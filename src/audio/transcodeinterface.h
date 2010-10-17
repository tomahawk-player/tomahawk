#ifndef TRANSCODEINTERFACE_H
#define TRANSCODEINTERFACE_H

#include <QStringList>
#include <QByteArray>
#include <QObject>
#include <QMutex>

class TranscodeInterface : public QObject
{
    Q_OBJECT

    public:
        virtual ~TranscodeInterface() {}

        virtual const QStringList supportedTypes() const = 0;

        virtual int needData() = 0;
        virtual bool haveData() = 0;

        virtual QByteArray data() = 0;

//        virtual void setBufferCapacity( int bytes ) = 0;
//        virtual int bufferSize() = 0;

    public slots:
        virtual void clearBuffers() = 0;
        virtual void onSeek( int seconds ) = 0;
        virtual void processData( const QByteArray& data, bool finish ) = 0;
};

#endif
