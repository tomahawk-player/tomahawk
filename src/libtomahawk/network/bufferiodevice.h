#ifndef BUFFERIODEVICE_H
#define BUFFERIODEVICE_H

#include <QIODevice>
#include <QMutexLocker>
#include <QDebug>
#include <QFile>

class BufferIODevice : public QIODevice
{
Q_OBJECT

public:
    explicit BufferIODevice( unsigned int size = 0, QObject *parent = 0 );

    virtual bool open( OpenMode mode );
    virtual void close();

    virtual bool seek( qint64 pos );
    void seeked( int block );

    virtual qint64 bytesAvailable() const;
    virtual qint64 size() const;
    virtual bool atEnd() const;
    virtual qint64 pos() const { qDebug() << Q_FUNC_INFO << m_pos; return m_pos; }

    void addData( int block, const QByteArray& ba );
    void clear();

    OpenMode openMode() const { qDebug() << "openMode"; return QIODevice::ReadOnly | QIODevice::Unbuffered; }

    void inputComplete( const QString& errmsg = "" );

    virtual bool isSequential() const { return false; }

    static unsigned int blockSize();

    int maxBlocks() const;
    int nextEmptyBlock() const;
    bool isBlockEmpty( int block ) const;

signals:
    void blockRequest( int block );

protected:
    virtual qint64 readData( char* data, qint64 maxSize );
    virtual qint64 writeData( const char* data, qint64 maxSize );

private:
    int blockForPos( qint64 pos ) const;
    int offsetForPos( qint64 pos ) const;
    QByteArray getData( qint64 pos, qint64 size );

    QList<QByteArray> m_buffer;
    mutable QMutex m_mut; //const methods need to lock
    unsigned int m_size, m_received;

    unsigned int m_pos;
};

#endif // BUFFERIODEVICE_H
