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

#ifndef BUFFERIODEVICE_H
#define BUFFERIODEVICE_H

#include <QIODevice>
#include <QMutexLocker>
#include <QFile>

class BufferIODevice : public QIODevice
{
Q_OBJECT

public:
    explicit BufferIODevice( unsigned int size = 0, QObject* parent = 0 );

    virtual bool open( OpenMode mode );
    virtual void close();

    virtual bool seek( qint64 pos );
    void seeked( int block );

    virtual qint64 bytesAvailable() const;
    virtual qint64 size() const;
    virtual bool atEnd() const;
    virtual qint64 pos() const { return m_pos; }

    void addData( int block, const QByteArray& ba );
    void clear();

    OpenMode openMode() const { return QIODevice::ReadOnly | QIODevice::Unbuffered; }

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
