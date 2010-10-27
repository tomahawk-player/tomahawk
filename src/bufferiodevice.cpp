#include <QDebug>
#include "bufferiodevice.h"


BufferIODevice::BufferIODevice( unsigned int size, QObject *parent ) :
    QIODevice( parent ),
    m_size( size ),
    m_received( 0 )
{
}


bool
BufferIODevice::open( OpenMode mode )
{
    QMutexLocker lock( &m_mut );

    qDebug() << Q_FUNC_INFO;
    QIODevice::open( QIODevice::ReadWrite ); // FIXME?
    return true;
}


void
BufferIODevice::close()
{
    QMutexLocker lock( &m_mut );

    qDebug() << Q_FUNC_INFO;
    QIODevice::close();
}


void
BufferIODevice::inputComplete( const QString& errmsg )
{
    qDebug() << Q_FUNC_INFO;
    setErrorString( errmsg );
    emit readChannelFinished();
}


void
BufferIODevice::addData( QByteArray ba )
{
    writeData( ba.data(), ba.length() );
}


qint64
BufferIODevice::bytesAvailable() const
{
    QMutexLocker lock( &m_mut );
    return m_buffer.length();
}


qint64
BufferIODevice::readData( char * data, qint64 maxSize )
{
    //qDebug() << Q_FUNC_INFO << maxSize;
    QMutexLocker lock( &m_mut );

    qint64 size = maxSize;
    if ( m_buffer.length() < maxSize )
        size = m_buffer.length();

    memcpy( data, m_buffer.data(), size );
    m_buffer.remove( 0, size );

    //qDebug() << "readData ends, bufersize:" << m_buffer.length();
    return size;
}


qint64 BufferIODevice::writeData( const char * data, qint64 maxSize )
{
    {
        QMutexLocker lock( &m_mut );
        m_buffer.append( data, maxSize );
        m_received += maxSize;
    }

    emit bytesWritten( maxSize );
    emit readyRead();
    return maxSize;
}


qint64 BufferIODevice::size() const
{
    return m_size;
}


bool BufferIODevice::atEnd() const
{
    QMutexLocker lock( &m_mut );
    return ( m_size == m_received && m_buffer.length() == 0 );
}


void
BufferIODevice::clear()
{
    QMutexLocker lock( &m_mut );
    m_buffer.clear();
}
