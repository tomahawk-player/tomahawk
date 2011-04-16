/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Leo Franchi <leo@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef DUMMYTRANSCODE_H
#define DUMMYTRANSCODE_H

#include "audio/transcodeinterface.h"
#include "dllmacro.h"

#define _BUFFER_PREFERRED 32768

class DLLEXPORT DummyTranscode : public TranscodeInterface
{
    Q_OBJECT

public:
    DummyTranscode();
    virtual ~DummyTranscode();

    const QStringList supportedTypes() const { QStringList l; l << "audio/basic"; return l; }

    int needData() { return true; } // always eats data
    bool haveData() { return !m_buffer.isEmpty(); }

    unsigned int preferredDataSize() { return _BUFFER_PREFERRED; }

    QByteArray data() { QByteArray b = m_buffer; m_buffer.clear(); return b; }

    virtual void setBufferCapacity( int bytes ) { m_bufferCapacity = bytes; }
    int bufferSize() { return m_buffer.size(); }

public slots:
    virtual void clearBuffers();
    virtual void onSeek( int seconds );
    virtual void processData( const QByteArray& data, bool finish );

signals:
    void streamInitialized( long sampleRate, int channels );
    void timeChanged( int seconds );

private:
    QByteArray m_buffer;
    int m_bufferCapacity;
    bool m_init;
};

#endif // DUMMYTRANSCODE_H
