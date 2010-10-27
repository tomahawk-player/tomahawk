#ifndef BUFFERIODEVICE_H
#define BUFFERIODEVICE_H

#include <QIODevice>
#include <QMutexLocker>
#include <QDebug>

class BufferIODevice : public QIODevice
{
Q_OBJECT
public:
    explicit BufferIODevice( unsigned int size = 0, QObject *parent = 0 );

    virtual bool open( OpenMode mode );
    virtual void close();

    virtual qint64 bytesAvailable() const;
    virtual qint64 size() const;
    virtual bool atEnd() const;

    void addData( QByteArray ba );
    void clear();

    OpenMode openMode() const { qDebug() << "openMode"; return QIODevice::ReadWrite; }

    void inputComplete( const QString& errmsg = "" );

protected:
    virtual qint64 readData( char * data, qint64 maxSize );
    virtual qint64 writeData( const char * data, qint64 maxSize );

private:
    QByteArray m_buffer;
    mutable QMutex m_mut; //const methods need to lock
    unsigned int m_size, m_received;
};

#endif // BUFFERIODEVICE_H
