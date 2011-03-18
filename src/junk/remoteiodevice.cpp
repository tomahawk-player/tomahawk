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

#include "remoteiodevice.h"

RemoteIODevice::RemoteIODevice(RemoteIOConnection * c)
    : m_eof(false), m_totalAdded(0), m_rioconn(c)
{
    qDebug() << "CTOR RemoteIODevice";
}

RemoteIODevice::~RemoteIODevice()
{
    qDebug() << "DTOR RemoteIODevice";
    m_rioconn->shutdown();
}

void RemoteIODevice::close()
{
    qDebug() << "RemoteIODevice::close";
    QIODevice::close();
    deleteLater();
}

bool RemoteIODevice::open ( OpenMode mode )
{
    return QIODevice::open(mode & QIODevice::ReadOnly);
}

qint64 RemoteIODevice::bytesAvailable () const
{
    return m_buffer.length();
}

bool RemoteIODevice::isSequential () const
{
    return true;
};

bool RemoteIODevice::atEnd() const
{
    return m_eof && m_buffer.length() == 0;
};

void RemoteIODevice::addData(QByteArray msg)
{
    m_mut_recv.lock();
    if(msg.length()==0)
    {
        m_eof=true;
        //qDebug() << "addData finished, entire file received. EOF.";
        m_mut_recv.unlock();
        m_wait.wakeAll();
        return;
    }
    else
    {
        m_buffer.append(msg);
        m_totalAdded += msg.length();
        //qDebug() << "RemoteIODevice has seen in total: " << m_totalAdded ;
        m_mut_recv.unlock();
        m_wait.wakeAll();
        emit readyRead();
        return;
    }
}

qint64 RemoteIODevice::writeData ( const char * data, qint64 maxSize )
{
    Q_ASSERT(false);
    return 0;
}

qint64 RemoteIODevice::readData ( char * data, qint64 maxSize )
{
    //qDebug() << "RemIO::readData, bytes in buffer: " << m_buffer.length();
    m_mut_recv.lock();
    if(m_eof && m_buffer.length() == 0)
    {
        // eof
        qDebug() << "readData called when EOF";
        m_mut_recv.unlock();
        return 0;
    }
    if(!m_buffer.length())// return 0;
    {
        //qDebug() << "WARNING readData when buffer is empty";
        m_mut_recv.unlock();
        return 0;
    }
    int len;
    if(maxSize>=m_buffer.length()) // whole buffer
    {
        len = m_buffer.length();
        memcpy(data, m_buffer.constData(), len);
        m_buffer.clear();
    } else { // partial
        len = maxSize;
        memcpy(data, m_buffer.constData(), len);
        m_buffer.remove(0,len);
    }
    m_mut_recv.unlock();
    return len;
}
