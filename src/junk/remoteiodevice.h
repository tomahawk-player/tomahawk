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
