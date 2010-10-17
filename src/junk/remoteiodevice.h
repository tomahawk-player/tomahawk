#ifndef REMOTEIODEVICE_H
#define REMOTEIODEVICE_H
#include <QIODevice>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QBuffer>
#include "remoteioconnection.h"

class RemoteIOConnection;

class RemoteIODevice : public QIODevice
{
    Q_OBJECT
public:

    RemoteIODevice(RemoteIOConnection * c);
    ~RemoteIODevice();
    virtual void close();
    virtual bool open ( OpenMode mode );
    qint64 bytesAvailable () const;
    virtual bool isSequential () const;
    virtual bool atEnd() const;

public slots:

    void addData(QByteArray msg);

protected:

    virtual qint64 writeData ( const char * data, qint64 maxSize );
    virtual qint64 readData ( char * data, qint64 maxSize );

private:
    QByteArray m_buffer;
    QMutex m_mut_wait, m_mut_recv;
    QWaitCondition m_wait;
    bool m_eof;
    int m_totalAdded;

    RemoteIOConnection * m_rioconn;
};
#endif // REMOTEIODEVICE_H
