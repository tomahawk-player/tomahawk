/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QUrl>
#include <QIODevice>

class DLLEXPORT MediaStream : public QObject
{
    Q_OBJECT

public:
    enum MediaType {
        Unknown = -1,
        Empty = 0,
        Url = 1,
        Stream = 2,
        IODevice = 3
    };

    MediaStream( QObject* parent = nullptr );
    explicit MediaStream( const QUrl &url );
    explicit MediaStream( QIODevice* device, bool bufferingFinished = false );
    virtual ~MediaStream();

    MediaType type() const;
    QUrl url() const;

    void setStreamSize( qint64 size );
    qint64 streamSize() const;

    virtual void seekStream( qint64 offset ) { (void)offset; }
    virtual qint64 needData ( void** buffer ) { (void)buffer; return 0; }

    int readCallback( const char* cookie, int64_t* dts, int64_t* pts, unsigned* flags, size_t* bufferSize, void** buffer );
    int readDoneCallback ( const char *cookie, size_t bufferSize, void *buffer );
    static int seekCallback ( void *data, const uint64_t pos );

public slots:
    void bufferingFinished();

protected:
    void endOfData();

    MediaType m_type;
    QUrl m_url;
    QIODevice* m_ioDevice;

    bool m_started = false;
    bool m_bufferingFinished = false;
    bool m_eos = false;
    qint64 m_pos = 0;
    qint64 m_streamSize = 0;

    char m_buffer[1048576];
private:
    Q_DISABLE_COPY( MediaStream )
};

#endif // MEDIASTREAM_H
